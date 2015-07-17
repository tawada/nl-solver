#include "./main.hpp"
#include "./route.hpp"
#include "./route_intermid.hpp"
#include "./fixflag.hpp"

/*******************************************************/
/** グローバル変数定義 **/
/*******************************************************/

Board* board; // 対象ボード

int penalty_T; // penalty of "touch"
int penalty_C; // penalty of "cross"

vector<int> adj_num; // 固定線に隣接する数字を格納

//map<int,Cost> cost_table; // コスト表
//int calc_T; // 計算するとき用
//int calc_C; // 計算するとき用

// ボードの X と Y
int board_x;
int board_y;


int main(int argc, char *argv[]){
	
	// ファイルの読み込み
	if(argc != 3){
		cerr << "Usage: ./solver.exe InputFile Fixed" << endl;
		exit(1);
	}
	initialize(argv[1]); // 問題盤の生成
	printBoard(); // 問題盤の表示
	
	// 固定フラグの生成
	int fixed = atoi(argv[2]);
	if(fixed==1){
		generateFixFlag();
	}
	
	// 乱数の初期化
	srand((unsigned int)time(NULL));
	// ペナルティの初期化
	penalty_T = 0;
	penalty_C = 0;
	
	// 初期ルーティング
	for(int i=1;i<=board->getLineNum();i++){
		// 数字が隣接する場合スキップ
		if(board->line(i)->getHasLine() == false) continue;
		
		if(!routing(i)){
			cerr << "Cannot solve!! (error: 1)" << endl;
			exit(1);
		}
	}
	for(int i=1;i<=board->getLineNum();i++){
		// 数字が隣接する場合スキップ
		if(board->line(i)->getHasLine() == false) continue;
		
		recording(i);
	}
	
	// 中間ポートを利用するか？
	// 利用している間，ライン不通過回数をカウントしない
	bool use_intermediate_port = false;
	// 中間ポートに設定するマスとそれを利用する数字
	int inter_x, inter_y, inter_line;
	
	
	// 探索スタート!!
	for(int m=2;m<=O_LOOP;m++){ // 外ループ
	
		if(!use_intermediate_port){ // 中間ポートを利用しない場合
			cout << "loop " << (m-1) << endl;
			if(m>INIT){ resetCandidate(); }
		}
		else{ // 中間ポートを利用する場合
			cout << "loop " << (m-1) << "+" << endl;
		}
		
		// 解導出フラグ
		bool complete = false;
		
		for(int n=1;n<=I_LOOP;n++){ // 内ループ
			if(m>INIT && !use_intermediate_port){ checkLineNonPassed(); }
			int id = rand() % board->getLineNum() + 1;
			//cout << "(" << m << "," << n << ")Re-route Line" << id << endl;
			
			// 数字が隣接してる場合スキップ
			if (board->line(id)->getHasLine() == false) continue;
			
			// 経路の削除
			deleting(id);
			
			// ペナルティの設定
			penalty_T = (int)(NT * (rand() % m));
			penalty_C = (int)(NC * (rand() % m));
			
			// 経路の探索
			bool success = false;
			int count = 1;
			if((board->line(id))->isIntermediateUsed()){ // 中間ポート利用経路探索
				while(!success){
					if(++count>10) break; // 10回失敗したら諦める
					if(!(success = routingSourceToI(id))){
						continue;
					}
					success = routingIToSink(id);
				}
			}
			else{ // 通常経路探索
				success = routing(id);
				if(!success){
					cerr << "Cannot solve!! (error: 2)" << endl; // 失敗したらプログラム終了
					exit(1);
				}
			}
			
			// 経路の記録
			// 中間ポート利用に失敗した場合，通常経路探索した後に内ループ脱出
			if(count>10){ // 通常経路探索（中間ポート利用に失敗）
				success = routing(id);
				if(!success){
					cerr << "Cannot solve!! (error: 3)" << endl; // 失敗したらプログラム終了
					exit(1);
				}
				recording(id);
				break;
			}
			recording(id);
		
			// 終了判定（解導出できた場合，正解を出力）
			if(isFinished()){
				printSolution();
				complete = true;
				break;
			}
		}
		if(complete) break; // 正解出力後は外ループも脱出
		
		// 中間ポートを使用した次のループでは，中間ポートを利用しない
		if(use_intermediate_port){
			use_intermediate_port = false;
			board->line(inter_line)->setIntermediateUnuse();
			continue;
		}
		
		// 不通過マスの調査->中間ポートを利用するか？
		int candidate_count = 0; // 候補数
		inter_x = -1;
		inter_y = -1;
		for(int y=0;y<board->getSizeY();y++){
			for(int x=0;x<board->getSizeX();x++){
				Box* trgt_box = board->box(x,y);
				if(trgt_box->isCandidate()){
					candidate_count++;
					cout << "(" << x << "," << y << ")"; // 不通過マス
				}
			}
		}
		if(candidate_count==0) continue; // 候補数0なら利用しない
		cout << endl;
		
		// 候補の中から中間ポートに設定するマスをランダムに選択
		int c_d = rand() % candidate_count; // 選択は候補の中で何番目か？
		int n_d = 0; // 何番目なのかをカウントする用の変数
		
		bool flag = false; // 二重ループのためフラグが必要
		for(int y=0;y<board->getSizeY();y++){
			for(int x=0;x<board->getSizeX();x++){
				Box* trgt_box = board->box(x,y);
				if(!trgt_box->isCandidate()){
					continue;
				}
				if(n_d==c_d){
					flag = true;
					inter_x = x; inter_y = y;
					break;
				}
				n_d++;
			}
			if(flag) break;
		}
		
		// 中間ポートの四方を見て，中間ポートを利用する数字をランダムに選択
		checkCandidateLine(inter_x,inter_y); // 候補となるラインを調査
		
		candidate_count = 0; // 候補数
		inter_line = -1;
		for(int i=1;i<=board->getLineNum();i++){
			Line* trgt_line = board->line(i);
			if(trgt_line->isCandidate()){
				candidate_count++;
				if(candidate_count>1) cout << ", ";
				cout << i;
			}
		}
		cout << endl;
		
		c_d = rand() % candidate_count; // 選択は候補の中で何番目か？
		n_d = 0; // 何番目なのかをカウントする用の変数
		
		for(int i=1;i<=board->getLineNum();i++){
			Line* trgt_line = board->line(i);
			if(!trgt_line->isCandidate()) continue;
			if(n_d==c_d){
				inter_line = i;
				break;
			}
			n_d++;
		}
		
		cout << "Set (" << inter_x << "," << inter_y << ") InterPort of Line " << inter_line << endl;
		Line* line_i = board->line(inter_line);
		line_i->setIntermediateUse();
		line_i->setIntermediatePort(inter_x,inter_y);
		use_intermediate_port = true;
		// m--; // 使うかどうか思案中・・・
		
		
		// ペナルティ更新（旧）
		//penalty_T = (int)(NT * m);
		//penalty_C = (int)(NC * m);
	}
	
	
	// 解導出できなかった場合
	if(!isFinished()){
		for(int i=1;i<=board->getLineNum();i++){
			printLine(i);
		}
	}
	
	//デバッグ用
	//for(int y=0;y<board->getSizeY();y++){
		//for(int x=0;x<board->getSizeX();x++){
			//cout << "(" << x << "," << y << ") ";
			//Box* trgt_box = board->box(x,y);
			//cout << " N:" << trgt_box->getNorthNum();
			//cout << " E:" << trgt_box->getEastNum();
			//cout << " S:" << trgt_box->getSouthNum();
			//cout << " W:" << trgt_box->getWestNum();
			//cout << endl;
		//}
	//}
	
	//for(int i=1;i<=board->getLineNum();i++){
	//	calcCost(i);
	//}
	
	delete board;
	return 0;
}

/**
 * 問題盤の初期化
 * @args: 問題ファイル名
 */
void initialize(char* filename){

	ifstream ifs(filename);
	string str;
	
	if(ifs.fail()){
		cerr << "File do not exist.\n";
		exit(1);
	}
	
	int line_num;
	map<int,int> x_0, y_0, x_1, y_1;
	map<int,bool> adjacents; // 初期状態で数字が隣接している

	while(getline(ifs,str)){
		if(str.at(0) == '#') continue;
		else if(str.at(0) == 'S'){ // 盤面サイズの読み込み
			str.replace(str.find("S"),5,"");
			str.replace(str.find("X"),1," ");
			istringstream is(str);
			is >> board_x >> board_y;
		}
		else if(str.at(0) == 'L' && str.at(4) == '_'){ // ライン個数の読み込み
			str.replace(str.find("L"),9,"");
			istringstream is(str);
			is >> line_num;
		}
		else if(str.at(0) == 'L' && str.at(4) == '#'){ // ライン情報の読み込み
			str.replace(str.find("L"),5,"");
			int index;
			while((index=str.find("("))!=-1){
				str.replace(index,1,"");
			}
			while((index=str.find(")"))!=-1){
				str.replace(index,1,"");
			}
			while((index=str.find(","))!=-1){
				str.replace(index,1," ");
			}
			str.replace(str.find("-"),1," ");
			int i, a, b, c, d;
			istringstream is(str);
			is >> i >> a >> b >> c >> d;
			x_0[i] = a; y_0[i] = b; x_1[i] = c; y_1[i] = d;
			
			// 初期状態で数字が隣接しているか判断
			int dx = x_0[i] - x_1[i];
			int dy = y_0[i] - y_1[i];
			if ((dx == 0 && (dy == 1 || dy == -1)) || ((dx == 1 || dx == -1) && dy == 0)) {
				adjacents[i] = true;
			} else {
				adjacents[i] = false;
			}
		}
		else continue;
	}
	
	board = new Board(board_x,board_y,line_num);
	
	for(int i=1;i<=line_num;i++){
		Box* trgt_box_0 = board->box(x_0[i],y_0[i]);
		Box* trgt_box_1 = board->box(x_1[i],y_1[i]);
		trgt_box_0->setTypeNumber();
		trgt_box_1->setTypeNumber();
		trgt_box_0->setNumber(i);
		trgt_box_1->setNumber(i);
		Line* trgt_line = board->line(i);
		trgt_line->setSourcePort(x_0[i],y_0[i]);
		trgt_line->setSinkPort(x_1[i],y_1[i]);
		trgt_line->setHasLine(!adjacents[i]);
	}
	
	for (int y = 0; y < board_y; y++) {
		for (int x = 0; x < board_x; x++) {
			Box* trgt_box = board->box(x,y);
			if(!trgt_box->isTypeNumber()) trgt_box->setTypeBlank();
		}
	}
}

int getConnectedNumber(int x,int y){

	Box* trgt_box = board->box(x,y);
	
	queue<Search> qu;
	if(trgt_box->isNorthLineFixed()){
		Search trgt = {x,y-1,SOUTH,-1};
		qu.push(trgt);
	}
	if(trgt_box->isEastLineFixed()){
		Search trgt = {x+1,y,WEST,-1};
		qu.push(trgt);
	}
	if(trgt_box->isSouthLineFixed()){
		Search trgt = {x,y+1,NORTH,-1};
		qu.push(trgt);
	}
	if(trgt_box->isWestLineFixed()){
		Search trgt = {x-1,y,EAST,-1};
		qu.push(trgt);
	}
	
	while(!qu.empty()){
		
		Search trgt = qu.front();
		qu.pop();
		
		trgt_box = board->box(trgt.x,trgt.y);
		
		if(trgt_box->isTypeNumber()){
			return trgt_box->getNumber();
		}
		
		if(trgt_box->isTypeHalfFixed()){
			continue;
		}
		
		// 北方向
		if(trgt_box->isNorthLineFixed() && trgt.d!=NORTH){
			Search next = {trgt.x,trgt.y-1,SOUTH,trgt.d};
			qu.push(next);
		}
		// 東方向
		if(trgt_box->isEastLineFixed() && trgt.d!=EAST){
			Search next = {trgt.x+1,trgt.y,WEST,trgt.d};
			qu.push(next);
		}
		// 南方向
		if(trgt_box->isSouthLineFixed() && trgt.d!=SOUTH){
			Search next = {trgt.x,trgt.y+1,NORTH,trgt.d};
			qu.push(next);
		}
		// 西方向
		if(trgt_box->isWestLineFixed() && trgt.d!=WEST){
			Search next = {trgt.x-1,trgt.y,EAST,trgt.d};
			qu.push(next);
		}
	}
	
	return -1;
}

void setAdjacentNumberProc(int x,int y){

	adj_num.clear();
	Box* trgt_box = board->box(x,y);
	
	queue<Search> qu;
	if(trgt_box->isNorthLineFixed()){
		Search trgt = {x,y-1,SOUTH,-1};
		qu.push(trgt);
	}
	if(trgt_box->isEastLineFixed()){
		Search trgt = {x+1,y,WEST,-1};
		qu.push(trgt);
	}
	if(trgt_box->isSouthLineFixed()){
		Search trgt = {x,y+1,NORTH,-1};
		qu.push(trgt);
	}
	if(trgt_box->isWestLineFixed()){
		Search trgt = {x-1,y,EAST,-1};
		qu.push(trgt);
	}
	
	while(!qu.empty()){
		
		Search trgt = qu.front();
		qu.pop();
		
		trgt_box = board->box(trgt.x,trgt.y);
				
		if(trgt_box->isTypeNumber() || trgt_box->isTypeHalfFixed()){
			continue;
		}
		
		// 北方向
		if(trgt_box->isNorthLineFixed() && trgt.d!=NORTH){
			Search next = {trgt.x,trgt.y-1,SOUTH,trgt.d};
			qu.push(next);
			// NORTH & trgt.d に線が存在
			// ->それ以外の方向に数字があったら，それは隣接する数字
			setAdjacentNumber(trgt.x,trgt.y,NORTH,trgt.d);
		}
		// 東方向
		if(trgt_box->isEastLineFixed() && trgt.d!=EAST){
			Search next = {trgt.x+1,trgt.y,WEST,trgt.d};
			qu.push(next);
			// EAST & trgt.d に線が存在
			// ->それ以外の方向に数字があったら，それは隣接する数字
			setAdjacentNumber(trgt.x,trgt.y,EAST,trgt.d);
		}
		// 南方向
		if(trgt_box->isSouthLineFixed() && trgt.d!=SOUTH){
			Search next = {trgt.x,trgt.y+1,NORTH,trgt.d};
			qu.push(next);
			// SOUTH & trgt.d に線が存在
			// ->それ以外の方向に数字があったら，それは隣接する数字
			setAdjacentNumber(trgt.x,trgt.y,SOUTH,trgt.d);
		}
		// 西方向
		if(trgt_box->isWestLineFixed() && trgt.d!=WEST){
			Search next = {trgt.x-1,trgt.y,EAST,trgt.d};
			qu.push(next);
			// WEST & trgt.d に線が存在
			// ->それ以外の方向に数字があったら，それは隣接する数字
			setAdjacentNumber(trgt.x,trgt.y,WEST,trgt.d);
		}
	}
}

void setAdjacentNumber(int x,int y,int d_1,int d_2){

	// 北を調査するか？
	if(y!=0 && d_1!=NORTH && d_2!=NORTH){
		Box* trgt_box = board->box(x,y-1);
		if(trgt_box->isTypeNumber()){
			adj_num.push_back(trgt_box->getNumber());
		}
	}
	// 東を調査するか？
	if(x!=(board->getSizeX()-1) && d_1!=EAST && d_2!=EAST){
		Box* trgt_box = board->box(x+1,y);
		if(trgt_box->isTypeNumber()){
			adj_num.push_back(trgt_box->getNumber());
		}
	}
	// 南を調査するか？
	if(y!=(board->getSizeY()-1) && d_1!=SOUTH && d_2!=SOUTH){
		Box* trgt_box = board->box(x,y+1);
		if(trgt_box->isTypeNumber()){
			adj_num.push_back(trgt_box->getNumber());
		}
	}
	// 西を調査するか？
	if(x!=0 && d_1!=WEST && d_2!=WEST){
		Box* trgt_box = board->box(x-1,y);
		if(trgt_box->isTypeNumber()){
			adj_num.push_back(trgt_box->getNumber());
		}
	}
}

bool isFinished(){

	map<int,map<int,int> > for_check;
	for(int y=0;y<board->getSizeY();y++){
		for(int x=0;x<board->getSizeX();x++){
			for_check[y][x] = 0;
		}
	}
	for(int i=1;i<=board->getLineNum();i++){
		Line* trgt_line = board->line(i);
		vector<int>* trgt_track = trgt_line->getTrack();
		for(int j=0;j<(int)(trgt_track->size());j++){
			int point = (*trgt_track)[j];
			int point_x = point % board->getSizeX();
			int point_y = point / board->getSizeX();
			if(for_check[point_y][point_x] > 0){ return false; }
			else{ for_check[point_y][point_x] = 1; }
		}
	}
	
	return true;
}

// ライン不通過回数をリセット（全マスについて）
// 各ラインの中間ポート利用候補をリセット
void resetCandidate(){

	for(int y=0;y<board->getSizeY();y++){
		for(int x=0;x<board->getSizeX();x++){
			Box* trgt_box = board->box(x,y);
			trgt_box->resetSelfCount();
			trgt_box->setNonCandidate();
		}
	}
	
	for(int i=1;i<=board->getLineNum();i++){
		Line* trgt_line = board->line(i);
		trgt_line->setNonCandidate();
	}
}	

// ライン不通過回数をカウント（数字マス以外）
// 回数が一定割合以上なら，コメント出力<-
void checkLineNonPassed(){

	for(int y=0;y<board->getSizeY();y++){
		for(int x=0;x<board->getSizeX();x++){
			Box* trgt_box = board->box(x,y);
			if(trgt_box->isTypeNumber() || trgt_box->isTypeAllFixed()) continue;
			if(countLineNum(x,y)<1){
				trgt_box->incrementSelfCount();
			}
			
			if(trgt_box->getSelfCount()>LIMIT){
				//cout << "(" << x << "," << y << ")" << endl;
				trgt_box->setCandidate();
				trgt_box->resetSelfCount();
			}
		}
	}
}

void checkCandidateLine(int x,int y){
	
	int now_x, now_y;
	
	// 北方向へ
	now_x = x; now_y = y;
	while(now_y>0){
		now_y = now_y - 1;
		Box* trgt_box = board->box(now_x,now_y);
		if(trgt_box->isTypeNumber()){
			if(!trgt_box->isTypeAllFixed() || trgt_box->isSouthLineFixed()){
				Line* trgt_line = board->line(trgt_box->getNumber());
				trgt_line->setCandidate();
			}
			break;
		}
		if(countLineNum(now_x,now_y)>0){
			for(int i=1;i<=board->getLineNum();i++){
				Line* trgt_line = board->line(i);
				vector<int>* trgt_track = trgt_line->getTrack();
				for(int j=0;j<(int)(trgt_track->size());j++){
					int point = (*trgt_track)[j];
					int point_x = point % board->getSizeX();
					int point_y = point / board->getSizeX();
					if(point_x==now_x && point_y==now_y){
						trgt_line->setCandidate();
						break;
					}
				}
			}
			break;
		}
	}
	
	// 東方向へ
	now_x = x; now_y = y;
	while(now_x<(board->getSizeX()-1)){
		now_x = now_x + 1;
		Box* trgt_box = board->box(now_x,now_y);
		if(trgt_box->isTypeNumber()){
			if(!trgt_box->isTypeAllFixed() || trgt_box->isWestLineFixed()){
				Line* trgt_line = board->line(trgt_box->getNumber());
				trgt_line->setCandidate();
			}
			break;
		}
		if(countLineNum(now_x,now_y)>0){
			for(int i=1;i<=board->getLineNum();i++){
				Line* trgt_line = board->line(i);
				vector<int>* trgt_track = trgt_line->getTrack();
				for(int j=0;j<(int)(trgt_track->size());j++){
					int point = (*trgt_track)[j];
					int point_x = point % board->getSizeX();
					int point_y = point / board->getSizeX();
					if(point_x==now_x && point_y==now_y){
						trgt_line->setCandidate();
						break;
					}
				}
			}
			break;
		}
	}
	
	// 南方向へ
	now_x = x; now_y = y;
	while(now_y<(board->getSizeY()-1)){
		now_y = now_y + 1;
		Box* trgt_box = board->box(now_x,now_y);
		if(trgt_box->isTypeNumber()){
			if(!trgt_box->isTypeAllFixed() || trgt_box->isNorthLineFixed()){
				Line* trgt_line = board->line(trgt_box->getNumber());
				trgt_line->setCandidate();
			}
			break;
		}
		if(countLineNum(now_x,now_y)>0){
			for(int i=1;i<=board->getLineNum();i++){
				Line* trgt_line = board->line(i);
				vector<int>* trgt_track = trgt_line->getTrack();
				for(int j=0;j<(int)(trgt_track->size());j++){
					int point = (*trgt_track)[j];
					int point_x = point % board->getSizeX();
					int point_y = point / board->getSizeX();
					if(point_x==now_x && point_y==now_y){
						trgt_line->setCandidate();
						break;
					}
				}
			}
			break;
		}
	}
	
	// 西方向へ
	now_x = x; now_y = y;
	while(now_x>0){
		now_x = now_x - 1;
		Box* trgt_box = board->box(now_x,now_y);
		if(trgt_box->isTypeNumber()){
			if(!trgt_box->isTypeAllFixed() || trgt_box->isEastLineFixed()){
				Line* trgt_line = board->line(trgt_box->getNumber());
				trgt_line->setCandidate();
			}
			break;
		}
		if(countLineNum(now_x,now_y)>0){
			for(int i=1;i<=board->getLineNum();i++){
				Line* trgt_line = board->line(i);
				vector<int>* trgt_track = trgt_line->getTrack();
				for(int j=0;j<(int)(trgt_track->size());j++){
					int point = (*trgt_track)[j];
					int point_x = point % board->getSizeX();
					int point_y = point / board->getSizeX();
					if(point_x==now_x && point_y==now_y){
						trgt_line->setCandidate();
						break;
					}
				}
			}
			break;
		}
	}
}
