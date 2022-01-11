#include "float.h"
#ifdef WINDOWS
#include <time.h>
#else
#include <sys/time.h>
#endif
#include "MyAI.h"
#include <iostream>
#include <algorithm>
#include <assert.h>     /* assert */
#include <set>
#include <vector>
#include <tuple>
#include <utility>
#include <unordered_map>

using namespace std;


#define MAX_DEPTH 6

#define WIN 1.0 * 400000
#define DRAW 0.2 * 400000
#define LOSE 0.0
#define BONUS 0.3 * 400000
#define BONUS_MAX_PIECE 8
#define NOEATFLIP_LIMIT 60
#define POSITION_REPETITION_LIMIT 3

double OFFSET = 0;//(WIN + BONUS);
bool ISTURNSTART = false;
double TURNSTART_VAL = 0;
int GLOBALTURN = 0;
bool GLOBALFLAG = false;
double HARD_TIME_LIMIT = 1000; //ms
static tuple<int,int> MAX_TUPLE=make_tuple(3,-1);
static const double values[14] = {
	  50, 3000,  500, 1100, 2400,4900,11000,
	  50, 3000,  500, 1100, 2400,4900,11000
};
static const double hasPKval[9] = {
	11000,4900,2400,1100,500,240,110,50,20
};
static const double KhasPKval[6] = {
	11000,6000,4000,3000,2300,2000 // //40905, 29904, 18903,13902,11501,11500
};
enum MetaState {
	MUSTLOSE,
	HALFMUSTLOSE,
	HALFLOSE,
	UNDETERMINED,
	HALFWIN,
	HALFMUSTWIN,
	MUSTWIN,
};
MetaState CURSTATE = UNDETERMINED;
int THINK_DEPTH = 3;
int EXCHANGE_DEPTH = 7;
int N_RED_PIECES = 0;
int N_BLACK_PIECES = 0;
bool HASEAT = false;
bool BESTHASEAT = true;
bool LASTTURNBESTHASEAT = true;
bool THISTURNBESTHASEAT = true;

unordered_map<long long,tuple<double,double,double>>* transposition_table[32];



MyAI::MyAI(void){srand(time(NULL));}

MyAI::~MyAI(void){}

bool MyAI::protocol_version(const char* data[], char* response){
	strcpy(response, "1.1.0");
  return 0;
}

bool MyAI::name(const char* data[], char* response){
	strcpy(response, "MyAI");
	return 0;
}

bool MyAI::version(const char* data[], char* response){
	strcpy(response, "1.0.0");
	return 0;
}

bool MyAI::known_command(const char* data[], char* response){
  for(int i = 0; i < COMMAND_NUM; i++){
		if(!strcmp(data[0], commands_name[i])){
			strcpy(response, "true");
			return 0;
		}
	}
	strcpy(response, "false");
	return 0;
}

bool MyAI::list_commands(const char* data[], char* response){
  for(int i = 0; i < COMMAND_NUM; i++){
		strcat(response, commands_name[i]);
		if(i < COMMAND_NUM - 1){
			strcat(response, "\n");
		}
	}
	return 0;
}

bool MyAI::quit(const char* data[], char* response){
  fprintf(stderr, "Bye\n"); 
	return 0;
}

bool MyAI::boardsize(const char* data[], char* response){
  fprintf(stderr, "BoardSize: %s x %s\n", data[0], data[1]); 
	return 0;
}

bool MyAI::reset_board(const char* data[], char* response){
	this->Red_Time = -1; // unknown
	this->Black_Time = -1; // unknown
	this->initBoardState();
	return 0;
}

bool MyAI::num_repetition(const char* data[], char* response){
  return 0;
}

bool MyAI::num_moves_to_draw(const char* data[], char* response){
  return 0;
}

bool MyAI::move(const char* data[], char* response){
  char move[6];
	sprintf(move, "%s-%s", data[0], data[1]);
	this->MakeMove(&(this->main_chessboard), move);
	return 0;
}

bool MyAI::flip(const char* data[], char* response){
  char move[6];
	sprintf(move, "%s(%s)", data[0], data[1]);
	this->MakeMove(&(this->main_chessboard), move);
	return 0;
}

bool MyAI::genmove(const char* data[], char* response){
	// set color
	if(!strcmp(data[0], "red")){
		this->Color = RED;
	}else if(!strcmp(data[0], "black")){
		this->Color = BLACK;
	}else{
		this->Color = 2; 
	}
	// genmove
  char move[6];
	this->generateMove(move);
	sprintf(response, "%c%c %c%c", move[0], move[1], move[3], move[4]);
	return 0;
}

bool MyAI::game_over(const char* data[], char* response){
  fprintf(stderr, "Game Result: %s\n", data[0]); 
	return 0;
}

bool MyAI::ready(const char* data[], char* response){
  return 0;
}

bool MyAI::time_settings(const char* data[], char* response){
  return 0;
}

bool MyAI::time_left(const char* data[], char* response){
  if(!strcmp(data[0], "red")){
		sscanf(data[1], "%d", &(this->Red_Time));
	}else{
		sscanf(data[1], "%d", &(this->Black_Time));
	}
	fprintf(stderr, "Time Left(%s): %s\n", data[0], data[1]); 
	return 0;
}

// int MyAI::time_left(const char* data[]){
//   if(!strcmp(data[0], "red")){
// 		sscanf(data[1], "%d", &(this->Red_Time));
// 	}else{
// 		sscanf(data[1], "%d", &(this->Black_Time));
// 	}
// 	//fprintf(stderr, "Time Left(%s): %s\n", data[0], data[1]); 
// 	return 0;
// }

bool MyAI::showboard(const char* data[], char* response){
  Pirnf_Chessboard();
	return 0;
}

bool MyAI::init_board(const char* data[], char* response){
  initBoardState(data);
	return 0;
}


// *********************** AI FUNCTION *********************** //

int MyAI::GetFin(char c)
{
	static const char skind[]={'-','K','G','M','R','N','C','P','X','k','g','m','r','n','c','p'};
	for(int f=0;f<16;f++)if(c==skind[f])return f;
	return -1;
}

int MyAI::ConvertChessNo(int input)
{
	switch(input)
	{
	case 0:
		return CHESS_EMPTY;
		break;
	case 8:
		return CHESS_COVER;
		break;
	case 1:
		return 6;
		break;
	case 2:
		return 5;
		break;
	case 3:
		return 4;
		break;
	case 4:
		return 3;
		break;
	case 5:
		return 2;
		break;
	case 6:
		return 1;
		break;
	case 7:
		return 0;
		break;
	case 9:
		return 13;
		break;
	case 10:
		return 12;
		break;
	case 11:
		return 11;
		break;
	case 12:
		return 10;
		break;
	case 13:
		return 9;
		break;
	case 14:
		return 8;
		break;
	case 15:
		return 7;
		break;
	}
	return -1;
}

long long zobrist[32][15];
long long zobrist_cover[14][6];
long long zobrist_turn[2];
long long hasher(ChessBoard* chessboard){
	long long ret = 0;
	for(int i = 0; i < 32; i++){
		if(chessboard -> Board[i]==CHESS_COVER){
			ret ^= zobrist[i][14];
		} else if (chessboard -> Board[i]!=CHESS_COVER){
			ret ^= zobrist[i][chessboard -> Board[i]];
		}
	}
	for (int i = 0; i < 14; ++i){
		ret ^= zobrist_cover[i][chessboard -> CoverChess[i]];
	}

	//ret ^= zobrist_turn[chessboard -> HistoryCount % 2];
	ret -= ret % 2;
	ret += chessboard -> HistoryCount % 2;
	//ret += chessboard -> HistoryCount;
	return ret;
}
bool hash_collision(ChessBoard* chessboard,int depth,tuple<double,double,double>* sol){
	int remain_depth = THINK_DEPTH-depth;
	if (remain_depth < 0)
		return false;
	long long h = 0;
	for(int i = 0; i < 32; i++){
		if(chessboard -> Board[i]==CHESS_COVER){
			h ^= zobrist[i][14];
		} else if (chessboard -> Board[i]!=CHESS_COVER){
			h ^= zobrist[i][chessboard -> Board[i]];
		}
	}
	for (int i = 0; i < 14; ++i){
		h ^= zobrist_cover[i][chessboard -> CoverChess[i]];
	}
	h -= h % 2;
	h += chessboard -> HistoryCount % 2; // ensure player turn
	for (int depth_bucket = 31; depth_bucket >= remain_depth; --depth_bucket ){
		//fprintf(stderr,"%d\n",depth_bucket);
		auto it = transposition_table[depth_bucket] -> find(h);
		if (it != transposition_table[depth_bucket] -> end()){
			//found hash
			*sol = it->second;
			return true;
		}
	}
	return false;
}

void MyAI::initBoardState()
{	
	int iPieceCount[14] = {5, 2, 2, 2, 2, 2, 1, 5, 2, 2, 2, 2, 2, 1};
	memcpy(main_chessboard.CoverChess, iPieceCount, sizeof(int)*14);
	main_chessboard.Red_Chess_Num = 16;
	main_chessboard.Black_Chess_Num = 16;
	main_chessboard.NoEatFlip = 0;
	main_chessboard.HistoryCount = 0;

	for(int i = 0; i < 32; i++){
		for(int j = 0; j < 15; j++){
			zobrist[i][j] = rand();
		}
	}
	for(int i = 0; i < 14; i++){
		for(int j = 0; j < 6; j++){
			zobrist_cover[i][j] = rand();
		}
	}
	

	// convert to my format
	int Index = 0;
	for(int i = 0; i < 8; i++)
	{
		for(int j = 0; j < 4; j++)
		{
			main_chessboard.Board[Index] = CHESS_COVER;
			Index++;
		}
	}

	for (int i = 0; i < 32; ++ i){
		unordered_map<long long,tuple<double,double,double>> * table = new unordered_map<long long,tuple<double,double,double>> ;
		//unordered_map<long long,tuple<double,double,double>> table; //transposition_table;
		transposition_table[i] = table;
	}

	Pirnf_Chessboard();
}



void MyAI::initBoardState(const char* data[])
{	
	memset(main_chessboard.CoverChess, 0, sizeof(int)*14);

	main_chessboard.Red_Chess_Num = 0;
	main_chessboard.Black_Chess_Num = 0;
	main_chessboard.NoEatFlip = 0;
	main_chessboard.HistoryCount = 0;

	// set board
	int Index = 0;
	for(int i = 0; i < 8; i++)
	{
		for(int j = 0; j < 4; j++)
		{
			// convert to my format
			int chess = ConvertChessNo(GetFin(data[Index][0]));
			main_chessboard.Board[Index] = chess;
			if(chess != CHESS_COVER && chess != CHESS_EMPTY){
				main_chessboard.CoverChess[chess]--;
				(chess < 7 ? 
					main_chessboard.Red_Chess_Num : main_chessboard.Black_Chess_Num)++;
			}
			Index++;
		}
	}
	// set covered chess
	for(int i = 0; i < 14; i++){
		main_chessboard.CoverChess[i] += data[32 + i][0] - '0';
		(i < 7 ? 
			main_chessboard.Red_Chess_Num : main_chessboard.Black_Chess_Num)
			+= main_chessboard.CoverChess[i];
	}

	Pirnf_Chessboard();
}

double MyAI::ElapsedTime(){
	double elapsed; // ms
	
	// design for different os
#ifdef WINDOWS
	clock_t end = clock();
	elapsed = (end - begin);
#else
	struct timeval end;
	gettimeofday(&end, 0);
	long seconds = end.tv_sec - begin.tv_sec;
	long microseconds = end.tv_usec - begin.tv_usec;
	elapsed = (seconds*1000 + microseconds*1e-3);
#endif

	return elapsed;
}
void MyAI::Expand(const int* board, const int color,vector<Move2Strength>* Result, int last_eaten_pos=-1) // Flip does not count
{
	int ResultCount = 0;
	for(int i=0;i<32;i++)
	{
		int self_chess_no = board[i];
		if(board[i] >= 0 && board[i]/7 == color)
		{
			//Gun
			if(board[i] % 7 == 1)
			{
				int row = i/4;
				int col = i%4;
				for(int rowCount=row*4;rowCount<(row+1)*4;rowCount++)
				{
					if(Referee(board,i,rowCount,color))
					{
						//Result[ResultCount] = i*100+rowCount;
						int eaten_chess_no = board[rowCount];
						if (last_eaten_pos >= 0) {
							if (last_eaten_pos == rowCount)
								Result -> push_back(Move2Strength { i*100+rowCount, make_tuple(-eaten_chess_no,-self_chess_no)});
						} else {
							Result -> push_back(Move2Strength{ i*100+rowCount, make_tuple(-eaten_chess_no,-self_chess_no) }) ;
						}
						
						//ResultCount++;
					}
				}
				for(int colCount=col; colCount<32;colCount += 4)
				{
				
					if(Referee(board,i,colCount,color))
					{
						int eaten_chess_no = board[colCount];
						if (last_eaten_pos >= 0) {
							if (last_eaten_pos == colCount)
								Result -> push_back(Move2Strength { i*100+colCount, make_tuple(-eaten_chess_no,-self_chess_no)});
						} else {
							Result-> push_back(Move2Strength{ i*100+colCount, make_tuple(-eaten_chess_no,-self_chess_no) }) ;
						}
						
						//ResultCount++;
					}
				}
			}
			else
			{
				int Move[4] = {i-4,i+1,i+4,i-1};
				for(int k=0; k<4;k++)
				{
					int eaten_chess_no = board[Move[k]];
					if(Move[k] >= 0 && Move[k] < 32 && Referee(board,i,Move[k],color))
					{
						if (last_eaten_pos >= 0) {
							if (last_eaten_pos == Move[k])
								Result -> push_back(Move2Strength { i*100+Move[k], make_tuple(-eaten_chess_no,-self_chess_no)});
						} else {
							
							Result-> push_back(Move2Strength{ i*100+Move[k] , make_tuple(-eaten_chess_no,-self_chess_no) }) ;
						}
						
						//ResultCount++;
					} 
				}
			}
		
		};
	}
	//return ResultCount;
}


void MyAI::generateMove(char move[6])
{
#ifdef WINDOWS
	begin = clock();
#else
	gettimeofday(&begin, 0);
#endif
/* generateMove Call by reference: change src,dst */
	int StartPoint = 0;
	int EndPoint = 0;

	GLOBALTURN += 1;
	double myturnstarttime;
	if (this->Color == RED){
		myturnstarttime = (double)(this->Red_Time == -1? 900000 :this->Red_Time-60000);
		fprintf(stderr,"I am red, time left %lf\n",myturnstarttime);
	}
	else{
		myturnstarttime = (double)(this->Black_Time == -1? 900000 :this->Black_Time-60000) ;
		fprintf(stderr,"I am black, time left %lf\n",myturnstarttime);
	}
	//fprintf(stderr, "%lf %d %lf\n",myturnstarttime,(60-GLOBALTURN),(double)myturnstarttime/(60-GLOBALTURN));
	double assign_time_this_turn;
	if (myturnstarttime > 0)
		assign_time_this_turn = max((double)1000,(double)myturnstarttime/(double)max(10,90-this->main_chessboard.HistoryCount));
	else if (myturnstarttime > -50000)
		assign_time_this_turn = 50;
	else
		assign_time_this_turn = 5;
	//assign_time_this_turn =1000000;//DEBUG;
	fprintf(stderr,"Assigned %lf seconds this turn,%lf \n",assign_time_this_turn,(double)myturnstarttime/(double)(60-GLOBALTURN));
	HARD_TIME_LIMIT = assign_time_this_turn * 2; // If surpassed assigned time by twice the amount, force stop
	
	
	fprintf(stderr,"HistoryCount %d\n",this->main_chessboard.HistoryCount);
	for (int i = 0; i < 8; ++i){
		fprintf(stderr, "%d %d %d %d\n",main_chessboard.Board[i*4],main_chessboard.Board[i*4+1],main_chessboard.Board[i*4+2],main_chessboard.Board[i*4+3]);
	}
	fflush(stderr);
	//assert(false);
	ISTURNSTART = true;
	double v;
	double t;
	double t_temp;
	int best_move;
	int best_move_tmp;
	bool isInit = true;
	for (int i = 0; i < 32; ++i){
		if (this->main_chessboard.Board[i] != CHESS_COVER){
			isInit = false;
			break;
		}
	}
	if (isInit){
		t = 0;
		best_move = 101; 
	}else{
		v = Evaluate(&this->main_chessboard, 1, this->Color, 0,0);
		v -= OFFSET;
		if (v >= 125000)
			CURSTATE = MUSTWIN;
		else if (v >= 75000)
			CURSTATE = HALFMUSTWIN;
		else if (v >= 25000)
			CURSTATE = HALFWIN;
		else if (v <= -125000)
			CURSTATE = MUSTLOSE;
		else if (v <= -75000)
			CURSTATE = HALFMUSTLOSE;
		else if (v <= -25000)
			CURSTATE = HALFLOSE;
		else
			CURSTATE = UNDETERMINED; 
		TURNSTART_VAL = v;
		fprintf(stderr, "Curstate: %d %lf\n", CURSTATE,v);
		fflush(stderr);
		ISTURNSTART = false;

		int flips = 0;
		int possibles = 0;
		for (int i = 0;i < 14;++i){
			if (this->main_chessboard.CoverChess[i] != 0){
				possibles += 1;
			}
		}
		for (int i = 0;i < 32;++i){
			if (this->main_chessboard.Board[i] == CHESS_COVER){
				flips += possibles;
				best_move = i * 100 + i;
			}
		}

		vector<Move2Strength> Moves1;
		Expand(this->main_chessboard.Board, this->Color,&Moves1);
		if (Moves1.size())
			best_move = Moves1.front().move; // initialize with random move so to avoid running out of time

		int nmoves1 = Moves1.size() + flips;
		vector<Move2Strength> Moves2;
		Expand(this->main_chessboard.Board, this->Color^1,&Moves2);
		int nmoves2 = Moves2.size()+flips;
		double time_complexity = max(nmoves1,nmoves2);
		double avg_complexity = pow((double)nmoves1 * nmoves2, 0.5);
		fprintf(stderr,"%d,%d,%lf\n",nmoves1,nmoves2,time_complexity);
		//assert(false);
		t = -DBL_MAX;
		t_temp = -DBL_MAX;

		// iterative-deeping, start from 3, time limit = <TIME_LIMIT> sec
		this->node = 0;
		

		// run Nega-max
		THINK_DEPTH = 1;
		if (avg_complexity < 50)
			EXCHANGE_DEPTH = 1;
		else
			EXCHANGE_DEPTH = 7;
		int last_t_nodes = 1;
		while(true){
			
			t_temp = NegaScout(this->main_chessboard, &best_move_tmp, this->Color, 0, 0, -DBL_MAX, DBL_MAX,-1,0,-1);
			t_temp -= OFFSET; // rescale
			if(!isTimeUp()){
				if (t_temp >= WIN || THINK_DEPTH > 50){ 
					t = t_temp;
					best_move = best_move_tmp;
					fprintf(stderr,"Found winning/tie path!\n");
					break;
				} else {
					t = t_temp;
					best_move = best_move_tmp;
					double t_depth = ElapsedTime();
					fprintf(stderr, "Depth: %2d, Time: %lf, Node: %10d, Score: %+1.5lf, Best Move: %d,\n",
					THINK_DEPTH, t_depth, this->node, t_temp, best_move);

					int tot_states = 0;
					for (int bucket = 0;bucket < 32 ; ++bucket){
						tot_states += transposition_table[bucket] -> size();
					}
					fprintf(stderr,"Saved states: %d\n",tot_states);
					
					if (t_depth > assign_time_this_turn)
						break;
					double power = (double)(THINK_DEPTH+1)/THINK_DEPTH;
					double projected_t_nextdepth = t_depth * time_complexity;
					fprintf(stderr, "Projected Depth %2d Time: %lf\n",
					THINK_DEPTH+1, projected_t_nextdepth);
					if (projected_t_nextdepth > assign_time_this_turn) // projected next turn overtime, break early
						break;
					last_t_nodes = this->node;
					THINK_DEPTH += 1;
				}
			} else {
				fprintf(stderr,"Interrupt by exceeding time\n");
				break;
			}
			
			
		}
		//assert(false);
		delete transposition_table[0];
		delete transposition_table[1];
		for (int i = 0;i < 30; ++i){
			transposition_table[i] = transposition_table[i+2];
		}
		unordered_map<long long,tuple<double,double,double>> * table = new unordered_map<long long,tuple<double,double,double>> ;
		//unordered_map<long long,tuple<double,double,double>> table; //transposition_table;
		transposition_table[31] = table;
		table = new unordered_map<long long,tuple<double,double,double>> ;
		//unordered_map<long long,tuple<double,double,double>> table; //transposition_table;
		transposition_table[30] = table;
		
		
	}


	// replace the move and score
	StartPoint = best_move/100;
	EndPoint   = best_move%100;
	sprintf(move, "%c%c-%c%c",'a'+(StartPoint%4),'1'+(7-StartPoint/4),'a'+(EndPoint%4),'1'+(7-EndPoint/4));
	fprintf(stderr, "Depth: %2d, Node: %10d, Score: %+1.5lf, Move: %s\n",  
		THINK_DEPTH, node, t, move);
	fflush(stderr);
	
	char chess_Start[4]="";
	char chess_End[4]="";
	Pirnf_Chess(main_chessboard.Board[StartPoint],chess_Start);
	Pirnf_Chess(main_chessboard.Board[EndPoint],chess_End);
	printf("My result: \n--------------------------\n");
	printf("NegaScout: %lf (node: %d)\n", t, this->node);
	printf("(%d) -> (%d)\n",StartPoint,EndPoint);
	printf("<%s> -> <%s>\n",chess_Start,chess_End);
	printf("move:%s\n",move);
	printf("--------------------------\n");
	this->Pirnf_Chessboard();
}

int MyAI::MakeMoveAndReturn(ChessBoard* chessboard, const int move, const int chess){
	// return 0 -> MOVE
	// return 1 -> EAT
	// return 2 -> FLIP
	if (move < 0) { //flip random ~= no move
		chessboard->NoEatFlip = 0;
		chessboard->History[chessboard->HistoryCount++] = move;
		return -1;
	} 
	int is_eat = -1;
	int src = move/100, dst = move%100;
	if(src == dst){ // flip
		chessboard->Board[src] = chess;
		chessboard->CoverChess[chess]--;
		chessboard->NoEatFlip = 0;
		is_eat = 2;
	}else { // move
		
		if(chessboard->Board[dst] != CHESS_EMPTY){
			if(chessboard->Board[dst] / 7 == 0){ // red
				(chessboard->Red_Chess_Num)--;
			}else{ // black
				(chessboard->Black_Chess_Num)--;
			}
			chessboard->NoEatFlip = 0;
			is_eat = 1;
		}else{
			chessboard->NoEatFlip += 1;
			is_eat = 0;
		}
		assert(is_eat >= 0);
		chessboard->Board[dst] = chessboard->Board[src];
		chessboard->Board[src] = CHESS_EMPTY;
		
	}
	chessboard->History[chessboard->HistoryCount++] = move;
	return is_eat;
}

void MyAI::MakeMove(ChessBoard* chessboard, const int move, const int chess){
	int src = move/100, dst = move%100;
	if(src == dst){ // flip
		chessboard->Board[src] = chess;
		chessboard->CoverChess[chess]--;
		chessboard->NoEatFlip = 0;
	}else { // move
		if(chessboard->Board[dst] != CHESS_EMPTY){
			if(chessboard->Board[dst] / 7 == 0){ // red
				(chessboard->Red_Chess_Num)--;
			}else{ // black
				(chessboard->Black_Chess_Num)--;
			}
			chessboard->NoEatFlip = 0;
		}else{
			chessboard->NoEatFlip += 1;
		}
		chessboard->Board[dst] = chessboard->Board[src];
		chessboard->Board[src] = CHESS_EMPTY;
	}
	chessboard->History[chessboard->HistoryCount++] = move;
}

void MyAI::MakeMove(ChessBoard* chessboard, const char move[6])
{ 
	int src, dst, m;
	src = ('8'-move[1])*4+(move[0]-'a');
	if(move[2]=='('){ // flip
		m = src*100 + src;
		printf("# call flip(): flip(%d,%d) = %d\n",src, src, GetFin(move[3])); 
	}else { // move
		dst = ('8'-move[4])*4+(move[3]-'a');
		m = src*100 + dst;
		printf("# call move(): move : %d-%d \n", src, dst);
	}
	MakeMove(chessboard, m, ConvertChessNo(GetFin(move[3])));
	Pirnf_Chessboard();
}


// Referee
bool MyAI::Referee(const int* chess, const int from_location_no, const int to_location_no, const int UserId)
{
	// int MessageNo = 0;
	bool IsCurrent = true;
	int from_chess_no = chess[from_location_no];
	int to_chess_no = chess[to_location_no];
	int from_row = from_location_no / 4;
	int to_row = to_location_no / 4;
	int from_col = from_location_no % 4;
	int to_col = to_location_no % 4;

	if(from_chess_no < 0 || ( to_chess_no < 0 && to_chess_no != CHESS_EMPTY) )
	{  
		// MessageNo = 1;
		//strcat(Message,"**no chess can move**");
		//strcat(Message,"**can't move darkchess**");
		IsCurrent = false;
	}
	else if (from_chess_no >= 0 && from_chess_no / 7 != UserId)
	{
		// MessageNo = 2;
		//strcat(Message,"**not my chess**");
		IsCurrent = false;
	}
	else if((from_chess_no / 7 == to_chess_no / 7) && to_chess_no >= 0)
	{
		// MessageNo = 3;
		//strcat(Message,"**can't eat my self**");
		IsCurrent = false;
	}
	//check attack
	else if(to_chess_no == CHESS_EMPTY && abs(from_row-to_row) + abs(from_col-to_col)  == 1)//legal move
	{
		IsCurrent = true;
	}	
	else if(from_chess_no % 7 == 1 ) //judge gun
	{
		int row_gap = from_row-to_row;
		int col_gap = from_col-to_col;
		int between_Count = 0;
		//slant
		if(from_row-to_row == 0 || from_col-to_col == 0)
		{
			//row
			if(row_gap == 0) 
			{
				for(int i=1;i<abs(col_gap);i++)
				{
					int between_chess;
					if(col_gap>0)
						between_chess = chess[from_location_no-i] ;
					else
						between_chess = chess[from_location_no+i] ;
					if(between_chess != CHESS_EMPTY)
						between_Count++;
				}
			}
			//column
			else
			{
				for(int i=1;i<abs(row_gap);i++)
				{
					int between_chess;
					if(row_gap > 0)
						between_chess = chess[from_location_no-4*i] ;
					else
						between_chess = chess[from_location_no+4*i] ;
					if(between_chess != CHESS_EMPTY)
						between_Count++;
				}
				
			}
			
			if(between_Count != 1 )
			{
				// MessageNo = 4;
				//strcat(Message,"**gun can't eat opp without between one piece**");
				IsCurrent = false;
			}
			else if(to_chess_no == CHESS_EMPTY)
			{
				// MessageNo = 5;
				//strcat(Message,"**gun can't eat opp without between one piece**");
				IsCurrent = false;
			}
		}
		//slide
		else
		{
			// MessageNo = 6;
			//strcat(Message,"**cant slide**");
			IsCurrent = false;
		}
	}
	else // non gun
	{
		//judge pawn or king

		//distance
		if( abs(from_row-to_row) + abs(from_col-to_col)  > 1)
		{
			// MessageNo = 7;
			//strcat(Message,"**cant eat**");
			IsCurrent = false;
		}
		//judge pawn
		else if(from_chess_no % 7 == 0)
		{
			if(to_chess_no % 7 != 0 && to_chess_no % 7 != 6)
			{
				// MessageNo = 8;
				//strcat(Message,"**pawn only eat pawn and king**");
				IsCurrent = false;
			}
		}
		//judge king
		else if(from_chess_no % 7 == 6 && to_chess_no % 7 == 0)
		{
			// MessageNo = 9;
			//strcat(Message,"**king can't eat pawn**");
			IsCurrent = false;
		}
		else if(from_chess_no % 7 < to_chess_no% 7)
		{
			// MessageNo = 10;
			//strcat(Message,"**cant eat**");
			IsCurrent = false;
		}
	}
	return IsCurrent;
}

int winorlose(int* mypiece, int* opponentpiece) {
	int covered[7] = {0};
	// 0: pawn, 1:cannon, 2:horse, 3: car, 4:elephant, 5: shi, 6:king
	if ((mypiece[0] + mypiece[6]) >= 2){
		if (ISTURNSTART == true){
			fprintf(stderr,"my 0+6 can take down of opponent 6\n");
			//fprintf(stderr,"my largest");
			fflush(stderr);
		}
		covered[6] = 1;
	}
	if ((mypiece[0] + mypiece[2] + mypiece[3] + mypiece[4] + mypiece[5] ) >= 2 && (mypiece[2] + mypiece[3] + mypiece[4] + mypiece[5] ) >= 1){
		covered[0] = 5;
	}
	int largest = -1;
	int secondlargest = -1;

	for (int piece = 6; piece >= 0; --piece){
		if (mypiece[piece] >= 2){
			if (largest == -1){
				largest = piece;
				secondlargest = piece;

			} else {
				secondlargest = piece;
			}
			break;

		} else if (mypiece[piece] == 1){
			if (largest == -1){
				largest = piece;

			} else if (secondlargest == -1){
				secondlargest = piece;
				break;
			}

		}
	}
	if (ISTURNSTART == true){
		fprintf(stderr,"my largest is %d, second largest is %d\n",largest,secondlargest);
		//fprintf(stderr,"my largest");
		fflush(stderr);
	}
	if (largest == secondlargest){
		for (int piece = secondlargest-1; piece >= 1; --piece){
			covered[piece] = 2;
		}
	}
	else { // largest > secondlargest
		for (int piece = secondlargest; piece >= 1; --piece){
			covered[piece] = 2;
		}
	}
	bool canonlysqueezekill = false;
	bool cannotkill = false;

	if (covered[6] < opponentpiece[6]){
		canonlysqueezekill = true;

		if (mypiece[0] > 0){
			mypiece[0] -= 1;
			if (ISTURNSTART == true){
				fprintf(stderr,"my 0 take care of opponent 6\n");
				//fprintf(stderr,"my largest");
				fflush(stderr);
			}
		} else if (mypiece[6] > 0){
			mypiece[6] -= 1;
			if (ISTURNSTART == true){
				fprintf(stderr,"my 6 take care of opponent 6\n");
				//fprintf(stderr,"my largest");
				fflush(stderr);
			}
		} else {
			cannotkill=true;
			if (ISTURNSTART == true){
				fprintf(stderr,"I cannot take care of opponent 6\n");
				//fprintf(stderr,"my largest");
				fflush(stderr);
			}
		}
	}
	for (int piece = 5; piece >= 1; --piece){

		if (covered[piece] < opponentpiece[piece]){
			int needtotakecare = opponentpiece[piece] - covered[piece];
			canonlysqueezekill = true;
			//bool foundsqueezepiece = false;
			for (int largerpiece = 6; largerpiece >= piece; --largerpiece){
				while (mypiece[largerpiece] > 0){
					mypiece[largerpiece] -= 1;
					needtotakecare -= 1;
					//if (needtotakecare == 0)
						//foundsqueezepiece = true;
					if (ISTURNSTART == true){
						fprintf(stderr,"my %d take care of opponent %d\n",largerpiece,piece);
						//fprintf(stderr,"my largest");
						fflush(stderr);
					}
					if (needtotakecare == 0)
						break;

				}
				if (needtotakecare == 0)
					break;

			}
			if (needtotakecare != 0){
				if (ISTURNSTART == true){
					fprintf(stderr,"I cannot take care of opponent %d\n",piece);
					//fprintf(stderr,"my largest");
					fflush(stderr);
				}
				cannotkill = true;
				break;
			}
		}
	}
	if (cannotkill == true){
		return 0;
	} else if (canonlysqueezekill == true){
		for (int piece = 6; piece >= 0; --piece){ // there is still another piece to move to gain control of squeezing
			if (mypiece[piece] > 0){
				return 1;
			}
		}
		return 0;
	} else {
		return 2;
	}

}
int getState(int* mypiece,  int* opponentpiece) {
	int tempmypiece[7] = {0};  
	int tempopponentpiece[7] = {0};
	for (int i = 0; i <= 6; ++i){
		tempmypiece[i] = mypiece[i];
		tempopponentpiece[i] = opponentpiece[i];
	}  

	int mystate = winorlose(tempmypiece, tempopponentpiece);
	if (ISTURNSTART == true){
		fprintf(stderr,"Can I beat opponent? state=%d\n",mystate);
		//fprintf(stderr,"my largest");
		fflush(stderr); 
	}

	for (int i = 0; i <= 6; ++i){
		tempmypiece[i] = mypiece[i];
		tempopponentpiece[i] = opponentpiece[i];
	} 
	int opponentstate = winorlose(tempopponentpiece, tempmypiece);
	if (ISTURNSTART == true){
		fprintf(stderr,"Can opponent beat me?=%d\n",opponentstate);
		//fprintf(stderr,"my largest");
		fflush(stderr); 
	}
	int nmypieces = 0;
	int nopponentpieces = 0;
	for (int p = 6; p >=0 ; --p){
		
		nmypieces += mypiece[p];
		nopponentpieces += opponentpiece[p];
	}
	if (mystate == 2 && opponentstate == 2){
		if (ISTURNSTART == true){
			fprintf(stderr,"NEUTRAL(ATTACK/ATTACK)\n");
			//fprintf(stderr,"my largest");
			fflush(stderr); 
		}
		return 0 - nopponentpieces * 500;
	}else if (mystate == 1 && opponentstate == 1){
		if (ISTURNSTART == true){
			fprintf(stderr,"SQUEEZE/SQUEEZE, opponent has %d pieces\n",nopponentpieces);
			//fprintf(stderr,"my largest");
			fflush(stderr); 
		}
		if (nopponentpieces <= 2)
			return 150000 -nopponentpieces * 500;
		else
			return -50000 + nmypieces* 500;
	}else if (mystate == 2 && opponentstate == 1){
		if (ISTURNSTART == true){
			fprintf(stderr,"ATTACK/SQUEEZE\n");
			//fprintf(stderr,"my largest");
			fflush(stderr); 
		}
		return 100000- nopponentpieces * 500;
	}else if (mystate == 1 && opponentstate == 0){
		if (ISTURNSTART == true){
			fprintf(stderr,"SQUEEZE/LOSE\n");
			//fprintf(stderr,"my largest");
			fflush(stderr); 
		}
		return 150000- nopponentpieces * 500;
	}else if (mystate == 2 && opponentstate == 0){
		if (ISTURNSTART == true){
			fprintf(stderr,"ATTACK/LOSE\n");
			//fprintf(stderr,"my largest");
			fflush(stderr); 
		}
		return 200000- nopponentpieces * 500;
	}else if (mystate == 1 && opponentstate == 2){
		if (ISTURNSTART == true){
			fprintf(stderr,"SQUEEZE/ATTACK\n");
			//fprintf(stderr,"my largest");
			fflush(stderr); 
		}
		return -100000 - nopponentpieces * 30;
	}else { // mystate == 0
		if (ISTURNSTART == true){
			fprintf(stderr,"LOSE/WHATEVER\n");
			//fprintf(stderr,"my largest");
			fflush(stderr); 
		}
		return -200000 + nmypieces * 500;
	}
}

// always use my point of view, so use this->Color
// double MyAI::EvaluateLight(const ChessBoard* chessboard,
// 	const int legal_move_count, const int color, int first_eat_bonus, int depth){
// }
double MyAI::EvaluateLight(const ChessBoard* chessboard, 
	const int legal_move_count, const int color, int first_eat_bonus, int depth){
	// score = My Score - Opponent's Score
	// offset = <WIN + BONUS> to let score always not less than zero

	double score = OFFSET;
	bool finish;

	if(legal_move_count == 0){ // Win, Lose
		if(color == this->Color){ // Lose
			score += LOSE - WIN;
		}else{ // Win
			score += WIN - LOSE;
		}
		finish = true;
	}else{ // no conclusion
		// static material values
		// cover and empty are both zero
		static const double values[14] = {
			  1,180,  6, 18, 90,270,810,  
			  1,180,  6, 18, 90,270,810
		};
		
		double piece_value = 0;
		for(int i = 0; i < 32; i++){
			if(chessboard->Board[i] != CHESS_EMPTY && 
				chessboard->Board[i] != CHESS_COVER){
				if(chessboard->Board[i] / 7 == this->Color){
					piece_value += values[chessboard->Board[i]];
				}else{
					piece_value -= values[chessboard->Board[i]];
				}
			}
		}
		// linear map to (-<WIN>, <WIN>)
		// score max value = 1*5 + 180*2 + 6*2 + 18*2 + 90*2 + 270*2 + 810*1 = 1943
		// <ORIGINAL_SCORE> / <ORIGINAL_SCORE_MAX_VALUE> * (<WIN> - 0.01)
		piece_value = piece_value / 1943 * (WIN - 0.01);
		score += piece_value; 
		finish = false;
	}

	// Bonus (Only Win / Draw)
	if(finish){
		if((this->Color == RED && chessboard->Red_Chess_Num > chessboard->Black_Chess_Num) ||
			(this->Color == BLACK && chessboard->Red_Chess_Num < chessboard->Black_Chess_Num)){
			if(!(legal_move_count == 0 && color == this->Color)){ // except Lose
				double bonus = BONUS / BONUS_MAX_PIECE * 
					abs(chessboard->Red_Chess_Num - chessboard->Black_Chess_Num);
				if(bonus > BONUS){ bonus = BONUS; } // limit
				score += bonus;
			}
		}else if((this->Color == RED && chessboard->Red_Chess_Num < chessboard->Black_Chess_Num) ||
			(this->Color == BLACK && chessboard->Red_Chess_Num > chessboard->Black_Chess_Num)){
			if(!(legal_move_count == 0 && color != this->Color)){ // except Lose
				double bonus = BONUS / BONUS_MAX_PIECE * 
					abs(chessboard->Red_Chess_Num - chessboard->Black_Chess_Num);
				if(bonus > BONUS){ bonus = BONUS; } // limit
				score -= bonus;
			}
		}
	}
	
	return score;
}
double MyAI::Evaluate(const ChessBoard* chessboard,
	const int legal_move_count, const int color, int first_eat_bonus, int depth){
	// score = My Score - Opponent's Score
	// offset = <WIN + BONUS> to let score always not less than zero
	//return EvaluateLight(chessboard, 
	//legal_move_count, color, first_eat_bonus, depth);
	int opponent = this->Color==1?0:1;
	int pieces_moves[14] = {0};

	if (ISTURNSTART == true){
		N_RED_PIECES = chessboard->Red_Chess_Num;
		N_BLACK_PIECES = chessboard->Black_Chess_Num;
	}

	vector<Move2Strength> Moves1;
	Expand(chessboard->Board, this->Color,&Moves1); // overwrite extra_moves
	//fprintf(stderr, "moves count: %d \n", Moves1.size());
	for (vector<Move2Strength>::iterator it = Moves1.begin();it != Moves1.end(); ++it){

		auto thisevaluator = it -> evaluator;
		int thispiece = -get<1>(thisevaluator);
		//fprintf(stderr, "moved whichpiece: %d \n", thispiece);
		pieces_moves[thispiece] += 1;
	}
	//assert(false);
	//my_extra_moves = Moves1.size();

	vector<Move2Strength> Moves2;
	Expand(chessboard->Board, opponent,&Moves2);
	for (vector<Move2Strength>::iterator it = Moves2.begin();it != Moves2.end(); ++it){

		auto thisevaluator = it -> evaluator;
		int thispiece = -get<1>(thisevaluator);
		pieces_moves[thispiece] += 1;
	}
	//oppo_extra_moves = Moves2.size();
	double score = OFFSET;
	bool finish=false;
	//fprintf(stderr, "legal_move_count: %d \n", legal_move_count);
	double close2preyscore = 0;
	double clusterscore = 0;
	double opponent_extra_moves_score = 0;
	double my_extra_moves_score= 0;
	double piece_value = 0;

	if(legal_move_count == 0){ // Win, Lose
		if(color == this->Color){ // Lose
			score += LOSE - WIN;
		}else{ // Win
			score += WIN - LOSE;
		}

		finish = true;}
	else if(chessboard->NoEatFlip >= NOEATFLIP_LIMIT){
		// if(color == this->Color){ // Lose
		// 	score += DRAW - WIN;
		// }else{ // Win
		// 	score += WIN - DRAW ;
		// }
		score += DRAW - DRAW;
		//finish = true;
	}
	else{ // no conclusion
		// static material values
		// cover and empty are both zero

		int count_pieces[14] = {0} ;
		int has_enemy[14] = {0} ;

		int weakest_enemy_pos = -1;
		int weakest_enemy_piece = 7;
		vector<tuple<int,int>> strongest_my_piece_pos;
		vector<tuple<int,int>> enemy_piece_pos;
		int nopponentpieces = 0;
		int nmypieces = 0;

		for (int i = 0; i < 14; ++i){
			count_pieces[i] = chessboard->CoverChess[i];
			int p = i;
			nopponentpieces += count_pieces[i] * int(p/7 == opponent);
			nmypieces += count_pieces[i] * int(p/7 == this->Color);
			
		}

		for(int i = 0; i < 32; i++){
			if(chessboard->Board[i] != CHESS_EMPTY &&
				chessboard->Board[i] != CHESS_COVER){
				count_pieces[chessboard->Board[i]]++;
				int p = chessboard->Board[i];
				nopponentpieces += int(p/7 == opponent);
				nmypieces += int(p/7 == this->Color);
				if (p/7 == opponent && p%7 < weakest_enemy_piece) {
					weakest_enemy_pos = i;
					weakest_enemy_piece = p%7;

				}

				//if (LASTTURNBESTHASEAT == false)
				if (p/7 == opponent && nopponentpieces <= 2)
					enemy_piece_pos.push_back(make_tuple((p%7),i));
				else if (p/7 == this->Color && LASTTURNBESTHASEAT == false)
					strongest_my_piece_pos.push_back(make_tuple((p%7),i));
			}
		}
		//for(int i = 0; i < 14; ++i)
		//	fprintf(stderr,"%d\n",count_pieces[i]);
		sort(strongest_my_piece_pos.begin(), strongest_my_piece_pos.end(), [](tuple<int,int> a, tuple<int,int> b) { return b<a; });
		int cumsum = (count_pieces[this->Color*7+6] == 1?3:2);
		int strong_threshold = 0;
		for (int piece=6;piece>=0;--piece){
			cumsum -= count_pieces[this->Color*7+piece];
			if (cumsum <= 0){
				strong_threshold = piece;
				break;
			}
		}

		score += getState(&count_pieces[this->Color*7],&count_pieces[opponent*7]); //meta score, win/lose
		//fprintf(stderr,"State Score:%lf\n",score);
		if (((count_pieces[0] != 0 && count_pieces[13] != 0) || (count_pieces[7] != 0 && count_pieces[6] != 0) )&& !finish){ // pawn bite king
			// dynamic score
			has_enemy[6] += count_pieces[7]; // pawn bite king, king cannot bite pawn
			has_enemy[13] += count_pieces[0]; // pawn bite king, king cannot bite pawn
			has_enemy[0] -= count_pieces[13]; // pawn bite king, king cannot bite pawn
			has_enemy[7] -= count_pieces[6]; // pawn bite king, king cannot bite pawn
			int acc1 = 0;
			int acc2 = 0;
			for (int what_piece = 6; what_piece >= 1; --what_piece){ // has STRICT enemy, same value does not count, dynamic value
				acc1 += count_pieces[what_piece];
				has_enemy[(what_piece-1+7)] = acc1 > 8 ? 8 : acc1;
				acc2 += count_pieces[what_piece+7];
				has_enemy[what_piece-1] = acc2 > 8 ? 8 : acc2;
			}

			for (int p = 0; p < 14; ++p){
				
				if (p % 7 == 6) {
					// if (ISTURNSTART == true){
					// 	fprintf(stderr,"%d,%d,%lf\n",p,count_pieces[p],KhasPKval[has_enemy[p]] * count_pieces[p] * (p / 7 == this->Color?1:-1));
					// 	//fprintf(stderr,"my largest");
					// 	fflush(stderr);
					// }

					piece_value += KhasPKval[has_enemy[p]] * count_pieces[p] * (p / 7 == this->Color?1:-1);
					my_extra_moves_score       += KhasPKval[has_enemy[p]] * 0.001 *  pieces_moves[p] * (p / 7 == this->Color?1:0);
					opponent_extra_moves_score += KhasPKval[has_enemy[p]] * 0.001 *  pieces_moves[p] * (p / 7 == this->Color?0:1);
				}
				else if (p % 7 == 1){
					piece_value += 3000 * count_pieces[p] * (p / 7 == this->Color?1:-1);
					//my_extra_moves_score       += 3000 * 0.01 *  pieces_moves[p] * (p / 7 == this->Color?1:0); //mobility of gun is irrelavant
					//opponent_extra_moves_score += 3000 * 0.01 *  pieces_moves[p] * (p / 7 == this->Color?0:1);
				}
				else {
					// if (ISTURNSTART == true){
					// 	fprintf(stderr,"%d,%d,%lf\n",p,count_pieces[p],hasPKval[has_enemy[p]] * count_pieces[p] * (p / 7 == this->Color?1:-1));
					// 	//fprintf(stderr,"my largest");
					// 	fflush(stderr);
					// }
					piece_value += hasPKval[has_enemy[p]] * count_pieces[p] * (p / 7 == this->Color?1:-1);
					my_extra_moves_score       += hasPKval[has_enemy[p]] * 0.001 *  pieces_moves[p] * (p / 7 == this->Color?1:0);
					opponent_extra_moves_score += hasPKval[has_enemy[p]] * 0.001 *  pieces_moves[p] * (p / 7 == this->Color?0:1);
				}
			}
		} else { //static value

			for (int p = 13; p >=0 ; --p){

				piece_value += values[p] * count_pieces[p] * (p / 7 == this->Color?1:-1);
				my_extra_moves_score       += max(values[p] * 0.001,0.5)  *  pieces_moves[p] * (p / 7 == this->Color?1:0);
				opponent_extra_moves_score += max(values[p] * 0.001,0.5) *  pieces_moves[p] * (p / 7 == this->Color?0:1);
			}
		}
		//fprintf(stderr,"Score:%lf\n",score);
		if (score > -125000+OFFSET && chessboard->NoEatFlip >= 30){

			for (vector<tuple<int,int>>::iterator it=strongest_my_piece_pos.begin(); it != strongest_my_piece_pos.end();++it ){
				if (weakest_enemy_piece == 6){
					if (get<0>(*it) == 0 || get<0>(*it) == 6 ){
						int pos1 = get<1>(*it);
						int pos2 = weakest_enemy_pos;
						int dist = abs(pos1%4-pos2%4) + abs(pos1/4-pos2/4);
						close2preyscore += (get<0>(*it)+1) * pow(2 , (3-dist)); //* (20 - depth);
						close2preyscore += 0; //* (20 - depth);
					}

				} else if (weakest_enemy_piece == 0){
					if (get<0>(*it) != 6 ){
						int pos1 = get<1>(*it);
						int pos2 = weakest_enemy_pos;
						int dist = abs(pos1%4-pos2%4) + abs(pos1/4-pos2/4);
						close2preyscore += (get<0>(*it)+1) * pow(2 , (3-dist)); //* (20 - depth);
						close2preyscore += 0; //* (20 - depth);
					}
				} else if (weakest_enemy_piece <= get<0>(*it)){ //weakest_enemy_piece = 1,2,3,4,5
					int pos1 = get<1>(*it);
					int pos2 = weakest_enemy_pos;
					int dist = abs(pos1%4-pos2%4) + abs(pos1/4-pos2/4);
					close2preyscore += (get<0>(*it)+1) * pow(2 , (3-dist));//* (20 - depth);
					close2preyscore += 0; //* (20 - depth);
				}
			}
		}
		//fprintf(stderr,"Score:%lf\n",score);
		if (score > -125000+OFFSET && nopponentpieces > 2){
			for (vector<tuple<int,int>>::iterator it=strongest_my_piece_pos.begin(); it != strongest_my_piece_pos.end();++it ){
				int piece1 = get<0>(*it);
				int pos1 = get<1>(*it);
				int dist = abs(pos1%4-1.5) + abs(pos1/4-3.5);
				clusterscore += (piece1+1) * pow(2 , (3-dist));
				clusterscore += 0;
				if (piece1 < strong_threshold)
					break;
				// for (vector<tuple<int,int>>::iterator it2=it+1; it2 != strongest_my_piece_pos.end();++it2 ){
				// 	int piece2= get<0>(*it2);
				// 	int pos2 = get<1>(*it2);
				// 	if (piece2 < strong_threshold)
				// 		break;
				// 	clusterscore += (3 - 1*abs(pos1%4-pos2%4));//* (20 - depth);
				// 	clusterscore += (7- 1*abs(pos1/4-pos2/4));//* (20 - depth);

				// }

			}

		}
		// if (score > -125000+OFFSET && nopponentpieces <= 2){
		// 	for (vector<tuple<int,int>>::iterator it=enemy_piece_pos.begin(); it != enemy_piece_pos.end();++it ){
		// 		int piece1 = get<0>(*it);
		// 		int pos1 = get<1>(*it);
		// 		for (vector<tuple<int,int>>::iterator it2=strongest_my_piece_pos.begin(); it2 != strongest_my_piece_pos.end();++it2 ){
		// 			int piece2 = get<0>(*it2);
		// 			int pos2 = get<1>(*it2);
		// 			if (piece1 == 6){
		// 				if (piece2 == 0 || piece2 == 6 ){
		// 					close2preyscore += (3 - 1 * abs(pos1%4-pos2%4)); //* (20 - depth);
		// 					close2preyscore += (7-1*abs(pos1/4-pos2/4)); //* (20 - depth);
		// 				}

		// 			} else if (piece1 == 0){
		// 				if (piece2 != 6 ){
		// 					close2preyscore += (3 - 1 * abs(pos1%4-pos2%4)); //* (20 - depth);
		// 					close2preyscore += (7-1*abs(pos1/4-pos2/4)); //* (20 - depth);
		// 				}
		// 			} else if (piece1 <= piece2){ //weakest_enemy_piece = 1,2,3,4,5
		// 				close2preyscore += (3 - 1*abs(pos1%4-pos2%4));//* (20 - depth);
		// 				close2preyscore += (7- 1*abs(pos1/4-pos2/4));//* (20 - depth);
		// 			}

		// 		}
		// 	}
		// }

	}
	// Bonus (Only Win / Draw)
	double bonus = 0;
	if(finish){
		bonus += this->Color == RED?(BONUS / BONUS_MAX_PIECE *
				min(chessboard->Red_Chess_Num - chessboard->Black_Chess_Num,BONUS_MAX_PIECE)):(BONUS / BONUS_MAX_PIECE *
				min((-chessboard->Red_Chess_Num + chessboard->Black_Chess_Num),BONUS_MAX_PIECE));

	}



	if (score < -125000+OFFSET){ // losing, play for tie
		opponent_extra_moves_score = 0;
	}
	// if (score >= -125000+OFFSET){ // winning, play to win
	// 	my_extra_moves_score = 0;
	// }
	//sassert(close2preyscore==0);
	if (ISTURNSTART == true ){
		fprintf(stderr,"%lf,%lf,%lf,%lf,%lf,%lf,%lf\n", score, piece_value, bonus, my_extra_moves_score,opponent_extra_moves_score, close2preyscore, clusterscore);
		//fprintf(stderr,"my largest");
		fflush(stderr);
		
	}
	// if (score + piece_value + bonus + 0.5*my_extra_moves_score - opponent_extra_moves_score + close2preyscore + clusterscore >220000 and GLOBALFLAG == true){

	// 	for (int i = 0; i < 8; ++i){
	// 		fprintf(stderr, "%d %d %d %d\n",chessboard -> Board[i*4],chessboard ->Board[i*4+1],chessboard ->Board[i*4+2],chessboard ->Board[i*4+3]);
	// 	}
	// 	for (int i = 0; i < 14; ++i){
	// 		fprintf(stderr, "%d \n",chessboard -> CoverChess[i]);
	// 	}
	// 	assert(false);
	// }

	return score + piece_value + bonus + 0.5*my_extra_moves_score - opponent_extra_moves_score + close2preyscore + clusterscore;
}
long MyAI::get_relavant_positions(const ChessBoard chessboard, const int position){
	long relavant_positions=0;
	// adjacent
	if (chessboard.Board[position]%7 == this->Color){ // self piece look for protect positions
		if (position / 4 != 0 && position % 4 != 0){
			relavant_positions ^= 1 << (position - 5) ;
		}
		if (position / 4 != 0 && position % 4 != 3){
			relavant_positions ^= 1 << (position - 3) ;
		}
		if (position / 4 != 7 && position % 4 != 0){
			relavant_positions ^= 1 << (position + 3) ;
		}
		if (position / 4 != 7 && position % 4 != 3){
			relavant_positions ^= 1 << (position + 5) ;
		}
	}
	
	if (position / 4 != 0){
		//fprintf(stderr,"UP%d\n", (position - 4));
		relavant_positions ^= 1 << (position - 4) ;
	}
	if (position / 4 != 7){
		//fprintf(stderr,"DOWN%d\n", (position + 4));
		relavant_positions^= 1 << (position + 4);
	}
	if (position % 4 != 0){
		//fprintf(stderr,"LEFT%d\n", (position - 1));
		relavant_positions^= 1 << (position - 1);
	}
	if (position % 4 != 3){
		//fprintf(stderr,"RIGHT%d\n", (position +1));
		relavant_positions^= 1 << (position + 1);
	}
	//fprintf(stderr,"%ld\n", relavant_positions);
	// if gun eat
	int temp = position;
	int count = 0;
	while (temp / 4 > 0){
		temp -= 4;
		if (chessboard.Board[temp] != CHESS_EMPTY){
			count += 1;
		} 
		if (count == 2){
			//fprintf(stderr,"up %d\n", (temp));
			relavant_positions^= 1 << temp;
			break;
		}
	}
	temp = position;
	count = 0;
	while (temp / 4 < 7){
		temp += 4;
		if (chessboard.Board[temp] != CHESS_EMPTY){
			count += 1;
		} 
		if (count == 2){
			//fprintf(stderr,"down %d\n", (temp));
			relavant_positions ^= 1 << temp;
			break;
		}
	}
	temp = position;
	count = 0;
	while (temp % 4 > 0){
		temp -= 1;
		if (chessboard.Board[temp] != CHESS_EMPTY){
			count += 1;
		} 
		if (count == 2){
			//fprintf(stderr,"left %d\n", (temp));
			relavant_positions ^= 1 << temp;
			break;
		}
	}
	temp = position;
	count = 0;
	while (temp % 4 < 3){
		temp += 1;
		if (chessboard.Board[temp] != CHESS_EMPTY){
			count += 1;
		} 
		if (count == 2){
			//fprintf(stderr,"right %d\n", (temp));
			relavant_positions ^= 1 << temp;
			break;
		}
	}
	return relavant_positions;


}
bool MyAI::isTimeUp(){
	double elapsed; // ms
	
	// design for different os
#ifdef WINDOWS
	clock_t end = clock();
	elapsed = (end - begin);
#else
	struct timeval end;
	gettimeofday(&end, 0);
	long seconds = end.tv_sec - begin.tv_sec;
	long microseconds = end.tv_usec - begin.tv_usec;
	elapsed = (seconds*1000 + microseconds*1e-3);
#endif

	return elapsed >= HARD_TIME_LIMIT;
}
double MyAI::NegaScout(const ChessBoard chessboard, int* move, const int color, const int depth, const int last_move_type, double alpha, double beta, int first_eat_bonus, int acc_skips, int last_eaten_pos = -1){

	vector<Move2Strength> Moves;
	int Flip_Moves[32];
	int Chess[2048];
	int move_count = 0, flip_count = 0, remain_count = 0, remain_total = 0;

	// move
	//if (depth > max(EXCHANGE_DEPTH,THINK_DEPTH)){
	//	Expand(chessboard.Board, color, &Moves, last_eaten_pos);
	//} else {
	Expand(chessboard.Board, color, &Moves);
	//}
	
	move_count = Moves.size();
	// flip
	for(int j = 0; j < 14; j++){ // find remain chess
		if(chessboard.CoverChess[j] > 0){
			Chess[remain_count] = j;
			remain_count++;
			remain_total += chessboard.CoverChess[j];
		}
	}
	for(int i = 0; i < 32; i++){ // find cover
		if(chessboard.Board[i] == CHESS_COVER){
			Flip_Moves[flip_count] = i*100+i;
			flip_count++;
		}
	}
	//fprintf(stderr, "depth=%d, move_count = %d, flip_count = % d\n", depth,move_count,flip_count);
	// if ((depth >= max(EXCHANGE_DEPTH,THINK_DEPTH)) && last_move_type == -1){
	// 	this->node++;
	// 	double val = Evaluate(&chessboard,move_count+flip_count, color,first_eat_bonus,depth) * (depth&1 ? -1 : 1);	
	// 	return val;
	// }
	// if ((depth >= max(EXCHANGE_DEPTH,THINK_DEPTH)) && move_count==0 && flip_count){
	// 	this->node++;
	// 	double val = Evaluate(&chessboard,move_count+flip_count, color,first_eat_bonus,depth) * (depth&1 ? -1 : 1);	
	// 	return val;
	// }
	//fprintf(stderr,"%d\n",last_move_type);
	if (acc_skips >= 2){
		this->node++;
		double val = Evaluate(&chessboard,move_count+flip_count, color,first_eat_bonus,depth) * (depth&1 ? -1 : 1);
		return val;
	}
	if (depth > max(EXCHANGE_DEPTH,THINK_DEPTH) && !(last_move_type==1) && !(last_move_type==-1)){
		this->node++;
		double val = Evaluate(&chessboard,move_count+flip_count, color,first_eat_bonus,depth) * (depth&1 ? -1 : 1);	
		if(depth == 0 || abs(abs(val)-202845.40) < 0.01){
			for (int i = 0; i < 8; ++i){
				fprintf(stderr, "%d %d %d %d\n",chessboard.Board[i*4],chessboard.Board[i*4+1],chessboard.Board[i*4+2],chessboard.Board[i*4+3]);
			}
			//assert(false);
		}
		return val;
	}
	else if (depth > THINK_DEPTH && !(last_move_type==1) && !(last_move_type==0)&& !(last_move_type==-1)){
		this->node++;
		double val = Evaluate(&chessboard,move_count+flip_count, color,first_eat_bonus,depth) * (depth&1 ? -1 : 1);
		if(depth == 0 || abs(abs(val)-202845.40) < 0.01){
			for (int i = 0; i < 8; ++i){
				fprintf(stderr, "%d %d %d %d\n",chessboard.Board[i*4],chessboard.Board[i*4+1],chessboard.Board[i*4+2],chessboard.Board[i*4+3]);
			}
			//assert(false);
		}
		return val;
	}
	else if(
		isTimeUp() ||
		chessboard.Red_Chess_Num == 0 || // terminal node (no chess type)
		chessboard.Black_Chess_Num == 0 || // terminal node (no chess type)
		move_count+flip_count == 0 // terminal node (no move type)
		){
		this->node++;
		// if (this->node % 10000 == 0){
		// 	fprintf(stderr, "node_parsed %d\n", this->node);
		// }
		//fprintf(stderr, "node=%d\n", this->node);
		// odd: *-1, even: *1
		//fprintf(stderr, "depth=%d, move_count = %d, flip_count = % d\n", depth,move_count,flip_count);
		
		double val = Evaluate(&chessboard,move_count+flip_count, color,first_eat_bonus,depth) * (depth&1 ? -1 : 1);
		//assert (val < 1000000 && val > -1000000);

		// for (int i = 0; i < 8; ++i){
		// 	fprintf(stderr, "%d %d %d %d\n",chessboard.Board[i*4],chessboard.Board[i*4+1],chessboard.Board[i*4+2],chessboard.Board[i*4+3]);
		// 	fprintf(stderr, "%lf", val);
			
		// }
		// assert (false);
		return val;
	}else{
		double m = -DBL_MAX; // lower bound
		double n = beta; // upper bound
		int new_move;
		double t ;
		
		sort(Moves.begin(), Moves.end(), [](Move2Strength a, Move2Strength b) { return a.evaluator < b.evaluator; });
		// search deeper
		for (vector<Move2Strength>::iterator it = Moves.begin();it != Moves.end(); ++it){  // move

			int thismove = it -> move;
			if (GLOBALFLAG and depth == 1){
				fprintf(stderr,"%d\n",thismove);
			}
			auto thisevaluator = it -> evaluator;
			if (depth == 4){
				fprintf(stderr,"%d\n",thismove);
			}
			
			// if (get<0>(thisevaluator) <=0)
			// 	first_eat_bonus += (1-get<0>(thisevaluator))*(30-depth) * ((depth % 2 == 0)?1:-1);
			
			ChessBoard new_chessboard = chessboard;
			int move_type = MakeMoveAndReturn(&new_chessboard, thismove, 0); // 0: MOVE, 1: EAT, 2: FLIP
			// if (depth >= 10){
			// 	fprintf(stderr, "Depth : %d Is_eat=%d \n",depth,is_eat);
			// 	for (int i = 0; i < 8; ++i){
			// 		fprintf(stderr, "%d %d %d %d\n",new_chessboard.Board[i*4],new_chessboard.Board[i*4+1],new_chessboard.Board[i*4+2],new_chessboard.Board[i*4+3]);
			// 	}
				
			// 	fflush(stderr);
			// }
			// if (depth >= 13){
			// 	assert (false);
			// }
			
				
			//fprintf(stderr,"Move%d\n",thismove);
			tuple<double,double,double> hash_sol;
			
			if (isDraw(&new_chessboard)==true)
				t = (OFFSET - (125000)) * (depth&1 ? -1 : 1);
			else if (hash_collision(&new_chessboard,depth,&hash_sol)){
				double lb = get<0>(hash_sol); //lower bound
				double exact = get<1>(hash_sol); //exact
				double ub = get<2>(hash_sol); //upper bound
				//fprintf(stderr,"%lf %lf %lf\n",lb,exact,ub);
				//assert(false);
				if (exact != -DBL_MAX){
					t = exact;
				} else if (ub != DBL_MAX){
					//beta = min(beta,ub);
					
					t = -NegaScout(new_chessboard, &new_move, color^1, depth+1, move_type, -n, -std::max(alpha, m),first_eat_bonus,0,thismove%100);

				} else if (lb != -DBL_MAX){
					//alpha = max(alpha,lb);
					t = -NegaScout(new_chessboard, &new_move, color^1, depth+1, move_type, -n, -std::max(alpha, m),first_eat_bonus,0,thismove%100);

				} else {
					assert(false);
				}
				if (GLOBALFLAG){
					assert(false);
				}
			}
			else  { // If last move is eat, keep searching
				t = -NegaScout(new_chessboard, &new_move, color^1, depth+1, move_type, -n, -std::max(alpha, m),first_eat_bonus,0,thismove%100);
			} 
			// if (get<0>(thisevaluator) <=0)
			// 	first_eat_bonus -= (1-get<0>(thisevaluator))*(30-depth) * ((depth % 2 == 0)?1:-1);
			//alpha = max(alpha, t);
			if(depth == 0 || abs(abs(t)-202845.40) < 0.01 || (depth == 4)){
				fprintf(stderr, "depth %d MOVE %d score = %.2lf\n",depth, thismove, t);
				fflush(stderr);
				if (depth == 0 && thismove == 1115){
					assert(false);
				}
				if (depth == 5 && thismove == 2322){
					//assert(false);
				}
				// if (depth == 0 and thismove == 1519){
				// 	GLOBALFLAG = true;
				// }
				// 	for (int i = 0; i < 8; ++i){
				// 	fprintf(stderr, "%d %d %d %d\n",new_chessboard.Board[i*4],new_chessboard.Board[i*4+1],new_chessboard.Board[i*4+2],new_chessboard.Board[i*4+3]);
				// }
				//assert(false);
			}
			if(t > m){ 
				if (n == beta || THINK_DEPTH - depth < 3 || t >= beta){ // No scouting
					m = t;
					*move = thismove;

					// if(depth == 1){ fprintf(stderr, "%d score= %.2lf\n", *move, t); }
				} else { 
					m = -NegaScout(new_chessboard, &new_move, color^1, depth+1, move_type, -beta, -t, first_eat_bonus,0); // re-search
					
					*move = thismove;
				}	
			}
			if (alpha >= beta){
				break;
			}	
			if (m >= beta){
				if (THINK_DEPTH - depth >= 0){
					//assert (false);
					//auto table = *transposition_table[THINK_DEPTH - depth];
					//(*transposition_table[THINK_DEPTH - depth])[hasher(&new_chessboard)] = make_tuple(-DBL_MAX,-DBL_MAX,m);
				}
				 
				return m;
			}
			if (t != 125000 && t != -125000){
				if (THINK_DEPTH - depth >= 0){
					//fprintf(stderr,"%p\n",transposition_table[THINK_DEPTH - depth]);
					//(*transposition_table[THINK_DEPTH - depth])[hasher(&new_chessboard)] = make_tuple(-DBL_MAX,t,DBL_MAX);
					if (t > alpha){
						//auto table = *transposition_table[THINK_DEPTH - depth];
						//fprintf(stderr,"%p\n",&table);
						(*transposition_table[THINK_DEPTH - depth])[hasher(&new_chessboard)] = make_tuple(-DBL_MAX,t,DBL_MAX); 
					} else {
						//auto table = *transposition_table[THINK_DEPTH - depth];
						//(*transposition_table[THINK_DEPTH - depth])[hasher(&new_chessboard)] = make_tuple(t,-DBL_MAX,DBL_MAX); 
					}
					//fprintf(stderr,"%lu\n",transposition_table[THINK_DEPTH - depth] -> size());
					// for (auto it = *transposition_table[THINK_DEPTH - depth].begin(); it != *transposition_table[THINK_DEPTH - depth].end() ; ++ it){
					// 	fprintf)_
					// }
					//assert (false);
				}
				
			} 
			
			//n = std::max(alpha, m)+0.01;			
		}
		if ((depth >= THINK_DEPTH)&&flip_count){ // If last move is only flip, check if the opponent can capture, fastforward to exchange phase	
			ChessBoard new_chessboard = chessboard;
			int move_type = MakeMoveAndReturn(&new_chessboard, -1, 0); // fake move
			//fprintf(stderr,"fakemove %d\n",depth);
			
			
			t = -NegaScout(new_chessboard, &new_move, color^1,depth + 1 , -1, -n, -std::max(alpha, m),first_eat_bonus,acc_skips+1);
			//fprintf(stderr,"%lf\n",t);

			if(abs(abs(t)-202845.40) < 0.01){
				fprintf(stderr, "depth %d MOVE %d score = %.2lf\n",depth, -1, t);
				fprintf(stderr,"%lf,%lf\n",t,beta);
				fflush(stderr);

			}
			if(t > m){ 
				m = t;
				*move = -1;
			}
			if (t != 125000 && t != -125000){
				if (THINK_DEPTH - depth >= 0){
					if (t > alpha){
						(*transposition_table[THINK_DEPTH - depth])[hasher(&new_chessboard)] = make_tuple(-DBL_MAX,t,DBL_MAX); 
					}
				}
			} 
		} 
		// if (depth == 0 && m -TURNSTART_VAL > 20){
		// 	fprintf(stderr,"move %d returns value %lf > %lf \n",*move,m,TURNSTART_VAL);
		// 	//assert(false);
		// 	return m;
		// }
		//fprintf(stderr,"depth now:%d\n",depth);
		if (depth < THINK_DEPTH && flip_count){

			long relavant_positions = 0;
			for(int i = 0; i < 32; i++){ 
				if (chessboard.Board[i] != CHESS_EMPTY && chessboard.Board[i] != CHESS_COVER){
					relavant_positions |= get_relavant_positions(chessboard,i);
					//fprintf(stderr,"relavant %d -> %ld ",i,relavant_positions);
					//assert(false);
				}
			}
			
			int irrelavant_positions[32];
			int n_irrelavant_positions = 0;
			for(int i = 0; i < flip_count; i++){ // flip RELAVANT positions
					double total = 0;
					if (!((1 << (Flip_Moves[i]%100) ) & relavant_positions)){
						irrelavant_positions[n_irrelavant_positions] = (Flip_Moves[i]%100);
						++n_irrelavant_positions;
						continue;
					} 
					// if (depth == 0 && Flip_Moves[i] == 1616){
					// 	GLOBALFLAG = true;
					// 	fprintf(stderr,"*****GLOBALFLAG StartPoint*****\n");
					// 	// for (int i = 0; i < 8; ++i){
					// 	// 	fprintf(stderr, "%d %d %d %d\n",new_chessboard.Board[i*4],new_chessboard.Board[i*4+1],new_chessboard.Board[i*4+2],new_chessboard.Board[i*4+3]);
					// 	// }
					// }

					for(int k = 0; k < remain_count; k++){ // what pieces could covered chess be
						ChessBoard new_chessboard = chessboard;
						
						int move_type = MakeMoveAndReturn(&new_chessboard, Flip_Moves[i], Chess[k]);
						//fprintf(stderr,"do flip %d\n",Flip_Moves[i]);
						double t = -NegaScout(new_chessboard, &new_move, color^1, depth+1, move_type, -DBL_MAX, DBL_MAX, first_eat_bonus,0);
						// if (t > 1000000){
						// 	fprintf(stderr,"%lf",t);
						// 	assert(false);
						// }
						if(depth == 0){
							fprintf(stderr, "--relavant FLIP %d is %d score = %.2lf\n", Flip_Moves[i],Chess[k], t);
							
							fflush(stderr);
						}
						total += chessboard.CoverChess[Chess[k]] * t;
						// early break if 無力回天
						//double max_possible = total

					}
					if(depth <= 100){
						//fprintf(stderr, "DEPTH=%d, thisflip=%d, chess=%d \n",depth, Flip_Moves[i], Chess[0]);
						fflush(stderr);
					}
					assert (remain_total > 0);
					double E_score = (total / remain_total); // calculate the expect value of flip
					if(E_score > m){ 
						m = E_score;
						*move = Flip_Moves[i];
					}
					
					
					if(depth == 0){
						fprintf(stderr, "depth%d relavant FLIP %d score = %.2lf\n",depth, Flip_Moves[i], E_score);
						fflush(stderr);
					}
			}
			if (n_irrelavant_positions){
				random_shuffle(&irrelavant_positions[0], &irrelavant_positions[n_irrelavant_positions]);
				// flip IRRELAVANT, random sample one
				int rnd_flip_move = irrelavant_positions[0] * 100 + irrelavant_positions[0];
				double total = 0;
				for(int k = 0; k < remain_count; k++){ // what pieces could covered chess be
					ChessBoard new_chessboard = chessboard;
					
					MakeMove(&new_chessboard, rnd_flip_move, Chess[k]);
					
					double t = -NegaScout(new_chessboard, &new_move, color^1, depth+1, 2, -DBL_MAX, DBL_MAX, first_eat_bonus,0);
					// if (t > 1000000){
					// 	fprintf(stderr,"%lf",t);
					// 	assert(false);
					// }
					if(depth == 0){
						fprintf(stderr, "--irrelavant FLIP %d is %d score = %.2lf\n", rnd_flip_move,Chess[k], t);
						fflush(stderr);
					}
					total += chessboard.CoverChess[Chess[k]] * t;
					// early break if 無力回天
					//double max_possible = total

				}
				if(depth <= 100){
					//fprintf(stderr, "DEPTH=%d, thisflip=%d, chess=%d \n",depth, Flip_Moves[i], Chess[0]);
					fflush(stderr);
				}
				assert (remain_total > 0);
				double E_score = (total / remain_total); // calculate the expect value of flip
				if(E_score > m){ 
					m = E_score;
					*move = rnd_flip_move;
				}

				if(depth == 0){
					fprintf(stderr, "irrelavant FLIP %d score = %.2lf\n", rnd_flip_move, E_score);
					fflush(stderr);
				}
			}
			
				
			
			
		}
		
		return m;
	}
}

bool MyAI::isDraw(const ChessBoard* chessboard){
	// No Eat Flip
	if(chessboard->NoEatFlip >= NOEATFLIP_LIMIT){
		return true;
	}

	// Position Repetition
	int last_idx = chessboard->HistoryCount - 1;
	// -2: my previous ply
	int idx = last_idx - 2;
	// All ply must be move type
	int smallest_repetition_idx = last_idx - (chessboard->NoEatFlip / POSITION_REPETITION_LIMIT);
	// check loop
	while(idx >= 0 && idx >= smallest_repetition_idx){
		if(chessboard->History[idx] == chessboard->History[last_idx]){
			// how much ply compose one repetition
			int repetition_size = last_idx - idx;
			bool isEqual = true;
			for(int i = 1; i < POSITION_REPETITION_LIMIT && isEqual; ++i){
				for(int j = 0; j < repetition_size; ++j){
					int src_idx = last_idx - j;
					int checked_idx = last_idx - i*repetition_size - j;
					if(chessboard->History[src_idx] != chessboard->History[checked_idx]){
						isEqual = false;
						break;
					}
				}
			}
			if(isEqual){
				return true;
			}
		}
		idx -= 2;
	}

	return false;
}

bool MyAI::isFinish(const ChessBoard* chessboard, int move_count){
	return (
		chessboard->Red_Chess_Num == 0 || // terminal node (no chess type)
		chessboard->Black_Chess_Num == 0 || // terminal node (no chess type)
		move_count == 0 || // terminal node (no move type)
		isDraw(chessboard) // draw
	);
}

//Display chess board
void MyAI::Pirnf_Chessboard()
{	
	char Mes[1024] = "";
	char temp[1024];
	char myColor[10] = "";
	if(Color == -99)
		strcpy(myColor, "Unknown");
	else if(this->Color == RED)
		strcpy(myColor, "Red");
	else
		strcpy(myColor, "Black");
	sprintf(temp, "------------%s-------------\n", myColor);
	strcat(Mes, temp);
	strcat(Mes, "<8> ");
	for(int i = 0; i < 32; i++){
		if(i != 0 && i % 4 == 0){
			sprintf(temp, "\n<%d> ", 8-(i/4));
			strcat(Mes, temp);
		}
		char chess_name[4] = "";
		Pirnf_Chess(this->main_chessboard.Board[i], chess_name);
		sprintf(temp,"%5s", chess_name);
		strcat(Mes, temp);
	}
	strcat(Mes, "\n\n     ");
	for(int i = 0; i < 4; i++){
		sprintf(temp, " <%c> ", 'a'+i);
		strcat(Mes, temp);
	}
	strcat(Mes, "\n\n");
	printf("%s", Mes);
}


// Print chess
void MyAI::Pirnf_Chess(int chess_no,char *Result){
	// XX -> Empty
	if(chess_no == CHESS_EMPTY){	
		strcat(Result, " - ");
		return;
	}
	// OO -> DarkChess
	else if(chess_no == CHESS_COVER){
		strcat(Result, " X ");
		return;
	}

	switch(chess_no){
		case 0:
			strcat(Result, " P ");
			break;
		case 1:
			strcat(Result, " C ");
			break;
		case 2:
			strcat(Result, " N ");
			break;
		case 3:
			strcat(Result, " R ");
			break;
		case 4:
			strcat(Result, " M ");
			break;
		case 5:
			strcat(Result, " G ");
			break;
		case 6:
			strcat(Result, " K ");
			break;
		case 7:
			strcat(Result, " p ");
			break;
		case 8:
			strcat(Result, " c ");
			break;
		case 9:
			strcat(Result, " n ");
			break;
		case 10:
			strcat(Result, " r ");
			break;
		case 11:
			strcat(Result, " m ");
			break;
		case 12:
			strcat(Result, " g ");
			break;
		case 13:
			strcat(Result, " k ");
			break;
	}
}


