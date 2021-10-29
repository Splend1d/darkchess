#include "float.h"
#include "time.h"
#include "MyAI.h"
#include <iostream>
#include <algorithm>
#include <assert.h>     /* assert */
#include <set>
#include <vector>
#include <tuple>
#include <utility>

using namespace std;

double TIME_LIMIT=9.5;

#define WIN 1.0 * 400000
#define DRAW 0.2 * 400000
#define LOSE 0.0
#define BONUS 0.3 * 400000
#define BONUS_MAX_PIECE 8

#define OFFSET (WIN + BONUS)

#define NOEATFLIP_LIMIT 60
#define POSITION_REPETITION_LIMIT 2

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
bool ISTURNSTART = true;
int GLOBALTURN = 0;

MyAI::MyAI(void){}

MyAI::~MyAI(void){}

bool MyAI::protocol_version(const char* data[], char* response){
	strcpy(response, "1.0.0");
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

bool MyAI::showboard(const char* data[], char* response){
  Pirnf_Chessboard();
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


void MyAI::initBoardState()
{	
	int iPieceCount[14] = {5, 2, 2, 2, 2, 2, 1, 5, 2, 2, 2, 2, 2, 1};
	memcpy(main_chessboard.CoverChess,iPieceCount,sizeof(int)*14);
	main_chessboard.Red_Chess_Num = 16;
	main_chessboard.Black_Chess_Num = 16;
	main_chessboard.NoEatFlip = 0;
	main_chessboard.HistoryCount = 0;

	//convert to my format
	int Index = 0;
	for(int i=0;i<8;i++)
	{
		for(int j=0;j<4;j++)
		{
			main_chessboard.Board[Index] = CHESS_COVER;
			Index++;
		}
	}
	Pirnf_Chessboard();
}

void MyAI::generateMove(char move[6])
{
	/* generateMove Call by reference: change src,dst */
	int StartPoint = 0;
	int EndPoint = 0;
	GLOBALTURN += 1;

	double t = -DBL_MAX;
	begin = clock();
	// if (CURSTATE == UNDETERMINED) {
	ISTURNSTART = true;
	double v = Evaluate(&this->main_chessboard, 1, this->Color, 0, 0,0);
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
	fprintf(stderr, "Curstate: %d %lf\n", CURSTATE,v);
	fflush(stderr);
	ISTURNSTART = false;
	
	// }
	
	

	// iterative-deeping, start from 3, time limit = <TIME_LIMIT> sec // 4-this->Color -this->Color
	for(int depth = 4; ( (double)(clock() - begin) / CLOCKS_PER_SEC < TIME_LIMIT) && depth <= 50; depth+=2){ //If first , search even, else search odd 
		this->node = 0;
		int best_move_tmp; double t_tmp;

		// run Nega-max

		t_tmp = Nega_max(this->main_chessboard, &best_move_tmp, this->Color, 0, depth, -1000000, 1000000,&MAX_TUPLE,0,0,0,-1);
		t_tmp -= OFFSET; // rescale

		// check score
		// if search all nodes
		// replace the move and score
		if(!this->timeIsUp){
			if (t_tmp <= t && t >= WIN){
				
			} else{
				t = t_tmp;
				StartPoint = best_move_tmp/100;
				EndPoint   = best_move_tmp%100;
				//fprintf(stderr, "Not Time Up");
				sprintf(move, "%c%c-%c%c",'a'+(StartPoint%4),'1'+(7-StartPoint/4),'a'+(EndPoint%4),'1'+(7-EndPoint/4)); // store sting to s

			}
			
					}
		// U: Undone
		// D: Done
		fprintf(stderr, "[%c] Depth: %2d, Node: %10d, Score: %+1.5lf, Move: %s\n (%d)", (this->timeIsUp ? 'U' : 'D'), 
			depth, node, t, move,best_move_tmp);
		fflush(stderr);
		// if (depth == 2)
		// 	depth += 2;
		// game finish !!! but search for bonus
		// if(t >= WIN){

		// 	break;
		// }
	}
	//exit(0); //DEBUG
	
	char chess_Start[4]="";
	char chess_End[4]="";
	Pirnf_Chess(main_chessboard.Board[StartPoint],chess_Start);
	Pirnf_Chess(main_chessboard.Board[EndPoint],chess_End);
	printf("My result: \n--------------------------\n");
	printf("Nega max: %lf (node: %d)\n", t, this->node);
	printf("(%d) -> (%d)\n",StartPoint,EndPoint);
	printf("<%s> -> <%s>\n",chess_Start,chess_End);
	printf("move:%s\n",move);
	printf("--------------------------\n");
	this->Pirnf_Chessboard();
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

void MyAI::Expand(const int* board, const int color,vector<Move2Strength>* Result,int last_move,bool Q=false)
{
	//int ResultCount = 0;
	int lastendpos = last_move%100;
	for(int i=0;i<32;i++)
	{
		int eaten_chess_no = 0;
		int self_chess_no= 0;
		if(board[i] >= 0 && board[i]/7 == color)
		{
			//Gun
			if(board[i] % 7 == 1)
			{
				int row = i/4;
				int col = i%4;
				for(int rowCount=row*4;rowCount<(row+1)*4;rowCount++)
				{
					if(Referee(board,i,rowCount,color,&eaten_chess_no,&self_chess_no))
					{
						assert("This game does not have gun!" == "false");
						Result -> push_back(Move2Strength{ i*100+rowCount, make_tuple(-eaten_chess_no,-self_chess_no) }) ;
						//ResultCount++;
					}
				}
				for(int colCount=col; colCount<32;colCount += 4)
				{
				
					if(Referee(board,i,colCount,color,&eaten_chess_no,&self_chess_no))
					{
						assert("This game does not have gun!" == "false");
						Result -> push_back(Move2Strength{ i*100+colCount, make_tuple(-eaten_chess_no,-self_chess_no) });
						//ResultCount++;
					}
				}
			}
			else // not gun
			{
				int Move[4] = {i-4,i+1,i+4,i-1};
				for(int k=0; k<4;k++)
				{
					
					if(Move[k] >= 0 && Move[k] < 32 && Referee(board,i,Move[k],color,&eaten_chess_no,&self_chess_no))
					{
						//auto neg_move_reward = make_tuple(-eaten_chess_no,-self_chess_no);
						if (Q == false){
							if (eaten_chess_no > 0)
								self_chess_no = -self_chess_no;
							Result -> push_back(Move2Strength { i*100+Move[k], make_tuple(-eaten_chess_no,-self_chess_no)});
							
						}
						else {
							// if (eaten_chess_no < 0) //didnt eat dont consider
							// 	continue;
							// //if ( last_eaten_piece >= 0 )// last step is eat then consider && eaten_chess_no%7 > last_eaten_piece%7)//eaten chess value less than last eaten chess dont consider
							// 	//continue;

							// if (eaten_chess_no > 0)
							// 	self_chess_no = -self_chess_no;
							if (lastendpos == Move[k])
								Result -> push_back(Move2Strength { i*100+Move[k], make_tuple(-eaten_chess_no,-self_chess_no)});
						}
						
						
						//ResultCount++;
					}
				}
			}
		
		};
	}
	//return ResultCount;
}


// Referee
bool MyAI::Referee(const int* chess, const int from_location_no, const int to_location_no, const int UserId, int* eaten_chess_no,  int* self_chess_no)
{
	// int MessageNo = 0;
	bool IsCurrent = true;
	int from_chess_no = chess[from_location_no];
	int to_chess_no = chess[to_location_no];
	int from_row = from_location_no / 4;
	int to_row = to_location_no / 4;
	int from_col = from_location_no % 4;
	int to_col = to_location_no % 4;
	*eaten_chess_no = to_chess_no;
	*self_chess_no = from_chess_no;

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

// always use my point of view, so use this->Color
// double MyAI::Evaluate(const ChessBoard* chessboard, 
// 	const int legal_move_count, const int color){
// 	// score = My Score - Opponent's Score
// 	// offset = <WIN + BONUS> to let score always not less than zero

// 	double score = OFFSET;
// 	bool finish;

// 	if(legal_move_count == 0){ // Win, Lose
// 		if(color == this->Color){ // Lose
// 			score += LOSE - WIN;
// 		}else{ // Win
// 			score += WIN - LOSE;
// 		}
// 		finish = true;
// 	}else if(isDraw(chessboard)){ // Draw
// 		// score = DRAW - DRAW;
// 		if(color == this->Color){ // Lose
// 			score += DRAW - WIN;
// 		}else{ // Win
// 			score += WIN - DRAW ;
// 		}
// 		 // Draw is Almost Lose
// 		//assert (true == false); 
// 		finish = true;
// 	}else{ // no conclusion
// 		// static material values
// 		// cover and empty are both zero
// 		static const double values[14] = {
// 			  1,180,  6, 18, 90,270,810,  
// 			  1,180,  6, 18, 90,270,810
// 		};
		
// 		double piece_value = 0;
// 		for(int i = 0; i < 32; i++){
// 			if(chessboard->Board[i] != CHESS_EMPTY && 
// 				chessboard->Board[i] != CHESS_COVER){
// 				if(chessboard->Board[i] / 7 == this->Color){
// 					piece_value += values[chessboard->Board[i]];
// 				}else{
// 					piece_value -= values[chessboard->Board[i]];
// 				}
// 			}
// 		}
// 		// linear map to (-<WIN>, <WIN>)
// 		// score max value = 1*5 + 180*2 + 6*2 + 18*2 + 90*2 + 270*2 + 810*1 = 1943
// 		// <ORIGINAL_SCORE> / <ORIGINAL_SCORE_MAX_VALUE> * (<WIN> - 0.01)
// 		piece_value = piece_value / 1943 * (WIN - 0.01);
// 		score += piece_value; 
// 		finish = false;
// 	}

// 	// Bonus (Only Win / Draw)
// 	if(finish){
// 		if((this->Color == RED && chessboard->Red_Chess_Num > chessboard->Black_Chess_Num) ||
// 			(this->Color == BLACK && chessboard->Red_Chess_Num < chessboard->Black_Chess_Num)){
// 			if(!(legal_move_count == 0 && color == this->Color)){ // except Lose
// 				double bonus = BONUS / BONUS_MAX_PIECE * 
// 				(chessboard->Red_Chess_Num - chessboard->Black_Chess_Num);
// 				if(bonus > BONUS){ bonus = BONUS; } // limit
// 				score += bonus;
// 			}
// 		}else if((this->Color == RED && chessboard->Red_Chess_Num < chessboard->Black_Chess_Num) ||
// 			(this->Color == BLACK && chessboard->Red_Chess_Num > chessboard->Black_Chess_Num)){
// 			if(!(legal_move_count == 0 && color != this->Color)){ // except Lose
// 				double bonus = BONUS / BONUS_MAX_PIECE * 
// 				(chessboard->Red_Chess_Num - chessboard->Black_Chess_Num);
// 				if(bonus > BONUS){ bonus = BONUS; } // limit
// 				score -= bonus;
// 			}
// 		}
// 	}
	
// 	return score;
// }

bool isQuiescent(vector<Move2Strength>* mvptr){
	sort(mvptr->begin(), mvptr->end(), [](Move2Strength a, Move2Strength b) { return a.evaluator < b.evaluator; });
	if(get<0>((*mvptr)[0].evaluator) > 0)
		return true;
	return false;
}
// always use my point of view, so use this->Color
double MyAI::Evaluate(const ChessBoard* chessboard, 
	const int legal_move_count, const int color, int my_extra_moves, int oppo_extra_moves, int first_eat_bonus){
	// score = My Score - Opponent's Score
	// offset = <WIN + BONUS> to let score always not less than zero
	int opponent = this->Color==1?0:1;
	int pieces_moves[14] = {0};
	
	vector<Move2Strength> Moves1;
	Expand(chessboard->Board, this->Color,&Moves1, -2); // overwrite extra_moves
	for (vector<Move2Strength>::iterator it = Moves1.begin();it != Moves1.end(); ++it){
		
		auto thisevaluator = it -> evaluator;
		int thispiece = -get<1>(thisevaluator);
		pieces_moves[thispiece] += 1;
	}
	//my_extra_moves = Moves1.size(); 
	
	vector<Move2Strength> Moves2;
	Expand(chessboard->Board, opponent,&Moves2, -2);
	for (vector<Move2Strength>::iterator it = Moves2.begin();it != Moves2.end(); ++it){
		
		auto thisevaluator = it -> evaluator;
		int thispiece = -get<1>(thisevaluator);
		pieces_moves[thispiece] += 1;
	}
	//oppo_extra_moves = Moves2.size(); 
	double score = OFFSET;
	bool finish;
	//fprintf(stderr, "legal_move_count: %d \n", legal_move_count);
			
	if(legal_move_count == 0){ // Win, Lose
		if(color == this->Color){ // Lose
			score += LOSE - WIN;
		}else{ // Win
			score += WIN - LOSE;
		}
		//fprintf(stderr, "lose:%f",score); 
		//exit(0);
		finish = true;}
	else if(chessboard->NoEatFlip >= NOEATFLIP_LIMIT){
		// if(color == this->Color){ // Lose
		// 	score += DRAW - WIN;
		// }else{ // Win
		// 	score += WIN - DRAW ;
		// }
		score += DRAW - WIN;
		finish = true;
		//fprintf(stderr, "draw"); 
		//exit(0);
	}
	else{ // no conclusion
		// static material values
		// cover and empty are both zero
		// if (ISTURNSTART == false){
		// 	fprintf(stderr, "un"); 
		// }
		
		int count_pieces[14] = {0} ;
		int has_enemy[14] = {0} ;
		double piece_value = 0;
		
		for(int i = 0; i < 32; i++){
			if(chessboard->Board[i] != CHESS_EMPTY && 
				chessboard->Board[i] != CHESS_COVER){
				count_pieces[chessboard->Board[i]]++;	
			}
		}
		bool maybe_advance_state = false;
		bool always_advance_state = false;
		//if (CURSTATE == UNDETERMINED){ // find if state is winning or losing
			
			
		
		if (!((count_pieces[0] != 0 && count_pieces[13] != 0) || (count_pieces[7] != 0 && count_pieces[6] != 0))) { // No King Pawn Bite
			maybe_advance_state = true;
			always_advance_state = true;
			if (ISTURNSTART == true){
				fprintf(stderr,"No King Pawn Bite,%d,%d,%d,%d\n",count_pieces[0],count_pieces[13],count_pieces[7],count_pieces[6]);
			}
		} else if ((count_pieces[opponent*7] == 0 && count_pieces[this->Color*7+6] != 0)|| 
							 (count_pieces[this->Color*7] == 0 && count_pieces[opponent*7+6] != 0)) { // One side don't have pawn and the other side has king
			always_advance_state = true;
			maybe_advance_state = true;
			if (ISTURNSTART == true){
				fprintf(stderr,"One side don't have pawn and the other side has king\n");
			}
		} else if (count_pieces[this->Color*7+6] != 0 && count_pieces[opponent*7+6] == 0){ // I have King, opponent doesn't have
			maybe_advance_state = true;
		} else if (count_pieces[opponent*7+6] != 0 && count_pieces[this->Color*7+6] == 0) { // Opponent has King, I don't have
			maybe_advance_state = true;
		}
		

			
		//}
		MetaState temp_state = UNDETERMINED;
		if (maybe_advance_state == true){ 
				
				int gt_chess_no[2][7] = {0};
				int acc[2] = {0};
				int largest[2] = {-1,-1};
				int nonkinglargest[2] = {-1,-1};
				int hasking = -1;
				double opponent_extra_moves_score = 0;
				double my_extra_moves_score= 0; 
				for (int p = 13; p >=0 ; --p){
					if (p % 7 ==6 && always_advance_state == false) { // Consider will win or not if king dead
						hasking = count_pieces[p]==1?p/7:1;
						continue;
					} 
						
					piece_value += values[p] * count_pieces[p] * (p / 7 == this->Color?1:-1);
					my_extra_moves_score       += max(values[p] * 0.001,0.5)  *  pieces_moves[p] * (p / 7 == this->Color?1:0);
					opponent_extra_moves_score += max(values[p] * 0.001,0.5) *  pieces_moves[p] * (p / 7 == this->Color?0:1);
					acc[p/7] += count_pieces[p];
					gt_chess_no[p/7][p%7] = acc[p/7];
					if (largest[p/7] ==-1 && count_pieces[p] != 0)
						largest[p/7] = p%7;
					if (nonkinglargest[p/7] ==-1 && count_pieces[p] != 0 && p % 7 != 6)
						nonkinglargest[p/7] = p%7;
				}
				// for piece in range(5,-1,-1){
				// 	if (gt_chess_no[this->Color][piece] >= count_pieces[piece])
				// }
				

				if (largest[this->Color] > largest[opponent] && (hasking == -1 || hasking == this->Color) && acc[this->Color] >= 2){ // I am absolutely bigger
					temp_state = MUSTWIN;
					if (ISTURNSTART == true){
						fprintf(stderr,"MUSTWIN: my largest color %d> oppo. largest color%d\n",largest[this->Color],largest[opponent]);
						//fprintf(stderr,"my largest");
						fflush(stderr);
					}
				}
				else if (largest[this->Color] < largest[opponent] && (hasking == -1 || hasking == opponent) && acc[opponent] >= 2) // Opponent is absolutely bigger
					temp_state = MUSTLOSE;
				else if (count_pieces[this->Color*7+6] == 1 &&  count_pieces[opponent*7] == 0 && gt_chess_no[this->Color][nonkinglargest[opponent]] >= 2 &&(!(count_pieces[opponent*7+6] == 1 &&  count_pieces[this->Color*7] == 0))&& acc[this->Color] > 2) { // I have an uncontested king and opponent doesn't have
					temp_state = HALFMUSTWIN;
					if (ISTURNSTART == true){
						fprintf(stderr,"HALFMUSTWIN: my %d(%d) >= 2\n",gt_chess_no[this->Color][nonkinglargest[opponent]],nonkinglargest[opponent]);
						//fprintf(stderr,"my largest");
						fflush(stderr); 
					}
				}
				else if (count_pieces[opponent*7+6] == 1  &&  count_pieces[opponent*7] == 0 && gt_chess_no[opponent][nonkinglargest[this->Color]] >= 2  && (!(count_pieces[this->Color*7+6] == 1 &&  count_pieces[opponent*7] == 0)) && acc[opponent] > 2) // Opponent has an uncontested king and I don't have
					temp_state = HALFMUSTLOSE;
				else if (gt_chess_no[this->Color][largest[opponent]] > count_pieces[opponent*7+largest[opponent]] && (hasking == -1 || hasking == this->Color)) { // I am relatively bigger (even if my king is eaten)
					temp_state = MUSTWIN;
					if (ISTURNSTART == true){
						fprintf(stderr,"MUSTWIN: my %d(%d) > oppo. %d(%d)\n",gt_chess_no[this->Color][largest[opponent]],largest[opponent],count_pieces[opponent*7+largest[opponent]],largest[opponent]);
						//fprintf(stderr,"my largest");
						fflush(stderr); 
					}
				}
				else if (gt_chess_no[this->Color][largest[opponent]] == count_pieces[opponent*7+largest[opponent]] && acc[this->Color] > acc[opponent] && (hasking == -1 || hasking == this->Color)){ // I am relatively bigger by a small margin (even if my king is eaten)
					temp_state = HALFWIN;
				}
				else if (gt_chess_no[this->Color][largest[opponent]] == count_pieces[opponent*7+largest[opponent]] && acc[this->Color] < acc[opponent] && (hasking == -1 || hasking == opponent)){ // I am relatively smaller by a small margin (even if opponent's king is eaten)
					temp_state = HALFLOSE;
				}
				else if (gt_chess_no[opponent][largest[this->Color]] > count_pieces[this->Color*7+largest[this->Color]] && (hasking == -1 || hasking == opponent)) { // I am relatively smaller (even if opponent's king is eaten)
					// for (int i = 0;i < 14; ++i)
					// 	fprintf(stderr, "how many %d: %d \n",i,count_pieces[i]); //DEBUG
					// fprintf(stderr,"%d %d\n",largest[this->Color],largest[opponent]);//DEBUG
					// fprintf(stderr,"%d\n",gt_chess_no[opponent][largest[this->Color]]);
					// fprintf(stderr,"%d\n",count_pieces[largest[this->Color]]);
					// fflush(stderr);
					// exit(0);
					temp_state = MUSTLOSE;
				} 
				if (ISTURNSTART == true) {
					fprintf(stderr,"temp state: %d\n",temp_state);
					//fprintf(stderr,"my largest");
					fflush(stderr);
				}
				
				switch (temp_state){
					case MUSTWIN:
						score += 150000- acc[opponent] * 500;
						piece_value +=  (- opponent_extra_moves_score * 2 );
						break;

					case HALFMUSTWIN:
						score += 100000 - acc[opponent] * 500;
						piece_value +=  (- opponent_extra_moves_score * 2);
						break;

					case HALFWIN:
						score += 50000  - acc[opponent] * 500;
						piece_value +=  (- opponent_extra_moves_score);
						break;

					case HALFLOSE:
						score -= 50000;
						score +=  acc[this->Color] * 500; //-opponent_extra_moves_score ;
						break;

					case HALFMUSTLOSE:
						score -= 100000;
						score += acc[this->Color] * 500;//-opponent_extra_moves_score ;
						break;

					case MUSTLOSE:
						score -= 150000;
						score += acc[this->Color] * 500;//my_extra_moves_score +acc[this->Color] * 500;
						break;

					default: 
						break;
				}
				// if (ISTURNSTART == false){
				// 	fprintf(stderr, "%d,%lf",temp_state,score-OFFSET); 
				// }
				if (always_advance_state == true)
					return score + piece_value + first_eat_bonus;
				
		}
		


		if ((count_pieces[0] != 0 && count_pieces[13] != 0) || (count_pieces[7] != 0 && count_pieces[6] != 0)){ // pawn bite king
			piece_value = 0 ;// dynamic score
			has_enemy[6] += count_pieces[7]; // pawn bite king, king cannot bite pawn
			has_enemy[13] += count_pieces[0]; // pawn bite king, king cannot bite pawn
			has_enemy[0] -= count_pieces[13]; // pawn bite king, king cannot bite pawn
			has_enemy[7] -= count_pieces[6]; // pawn bite king, king cannot bite pawn
			int acc1 = 0;
			int acc2 = 0;
			for (int what_piece = 6; what_piece >= 1; --what_piece){ // has STRICT enemy, same value does not count
				acc1 += count_pieces[what_piece];
				has_enemy[(what_piece-1+7)] = acc1 > 8 ? 8 : acc1;
				acc2 += count_pieces[what_piece+7];
				has_enemy[what_piece-1] = acc2 > 8 ? 8 : acc2;
			}
			double my_extra_moves_score = 0;
			double opponent_extra_moves_score = 0;
			for (int p = 0; p < 14; ++p){
				if (p % 7 == 6) {
					piece_value += KhasPKval[has_enemy[p]] * count_pieces[p] * (p / 7 == this->Color?1:-1);
					my_extra_moves_score       += KhasPKval[has_enemy[p]] * 0.001 *  pieces_moves[p] * (p / 7 == this->Color?1:0);
					opponent_extra_moves_score += KhasPKval[has_enemy[p]] * 0.001 *  pieces_moves[p] * (p / 7 == this->Color?0:1);
				}
				else {
					piece_value += hasPKval[has_enemy[p]] * count_pieces[p] * (p / 7 == this->Color?1:-1);
					my_extra_moves_score       += hasPKval[has_enemy[p]] * 0.001 *  pieces_moves[p] * (p / 7 == this->Color?1:0);
					opponent_extra_moves_score += hasPKval[has_enemy[p]] * 0.001 *  pieces_moves[p] * (p / 7 == this->Color?0:1);
				}
			} 
			switch (temp_state){
					case MUSTWIN:
						//score += 150000;
						piece_value +=  (- opponent_extra_moves_score * 2 );
						break;

					case HALFMUSTWIN:
						//score += 100000;
						piece_value +=  (- opponent_extra_moves_score * 2 );
						break;

					case HALFWIN:
						//score += 50000;
						piece_value +=  (- opponent_extra_moves_score);
						break;

					

					default:
						score += (- opponent_extra_moves_score) ;
				}
			 // mobility score
			
		} 
		// else{ // no king pawn loop
		// 	int gt_chess_no[2][7] = {0};
		// 	int acc[2] = {0};
		// 	int largest[2] = {-1};
		// 	for (int p = 13; p >=0 ; --p){
		// 		piece_value += values[p] * count_pieces[p] * (p / 7 == this->Color?1:-1);
		// 		acc[p/7] += count_pieces[p];
		// 		gt_chess_no[p/7][p%7] = acc[p/7];
		// 		if (largest[p/7] ==-1 && count_pieces[p%7] != 0)
		// 			largest[p/7] = p%7;
		// 	}
		// 	bool mustwin = false;
		// 	bool halfwin = false;
		// 	bool halflose = false;
		// 	bool mustlose = false;
		// 	int opponent = this->Color==1?0:1;
		// 	if (gt_chess_no[this->Color][largest[opponent]] > largest[opponent])
		// 		mustwin = true;
		// 	else if (gt_chess_no[this->Color][largest[opponent]] == largest[opponent] && acc[this->Color] > acc[opponent]){
		// 		halfwin = true;
		// 	}
		// 	else if (gt_chess_no[this->Color][largest[opponent]] == largest[opponent] && acc[this->Color] < acc[opponent]){
		// 		halflose = true;
		// 	}
		// 	else if (gt_chess_no[opponent][largest[this->Color]] > largest[this->Color])
		// 		mustlose = true;
		// 	if (mustwin){
		// 		score += 50000;
		// 		score +=  - oppo_extra_moves * 50;
		// 	}
		// 	else if  (halfwin){
		// 		score += 25000;
		// 	}
		// 	else if  (halflose){
		// 		score += 25000;
		// 	}
		// 	else if (mustlose){
		// 		score -= 50000;
		// 		score +=  my_extra_moves;
		// 	}
		// }
		// DEBUG
		// for (int p = 0;p < 14; ++p){
		// 	if (p % 7 == 6)
		// 		fprintf(stderr, "Value%d: %lf \n", p,KhasPKval[has_enemy[p]] * count_pieces[p] * (p / 7 == this->Color?1:-1));
		// 	else
		// 		fprintf(stderr, "Value%d: %lf \n", p,hasPKval[has_enemy[p]] * count_pieces[p] * (p / 7 == this->Color?1:-1));
		// }
		
		
		// for (int i = 0;i < 14; ++i)
		// 	fprintf(stderr, "how many %d: %d \n",i,count_pieces[i]); //DEBUG
		// fprintf(stderr,"%lf\n",piece_value);//DEBUG
		// fflush(stderr);
		// exit(0);

		score += piece_value; 
		
		
		finish = false;

	}

	// Bonus (Only Win / Draw)
	if(finish){
		double bonus = this->Color == RED?(BONUS / BONUS_MAX_PIECE * 
				min(chessboard->Red_Chess_Num - chessboard->Black_Chess_Num,BONUS_MAX_PIECE)):(BONUS / BONUS_MAX_PIECE * 
				min((-chessboard->Red_Chess_Num + chessboard->Black_Chess_Num),BONUS_MAX_PIECE));
		score += bonus;
		// if((this->Color == RED && chessboard->Red_Chess_Num > chessboard->Black_Chess_Num) ||
		// 	(this->Color == BLACK && chessboard->Red_Chess_Num < chessboard->Black_Chess_Num)){
		// 	if(!(legal_move_count == 0 && color == this->Color)){ // except Lose, we don't care lose
		// 		double bonus = BONUS / BONUS_MAX_PIECE * 
		// 		(chessboard->Red_Chess_Num - chessboard->Black_Chess_Num);
		// 		if(bonus > BONUS){ bonus = BONUS; } // limit
		// 		score += bonus;
		// 	}
		// }else if((this->Color == RED && chessboard->Red_Chess_Num < chessboard->Black_Chess_Num) ||
		// 	(this->Color == BLACK && chessboard->Red_Chess_Num > chessboard->Black_Chess_Num)){
		// 	if(!(legal_move_count == 0 && color != this->Color)){ // except Lose, we don't care lose
		// 		double bonus = BONUS / BONUS_MAX_PIECE * 
		// 		(chessboard->Red_Chess_Num - chessboard->Black_Chess_Num);
		// 		if(bonus > BONUS){ bonus = BONUS; } // limit
		// 		score -= bonus;
		// 	}
		// }
	}
	
	return score + first_eat_bonus;
}

double MyAI::Nega_max(const ChessBoard chessboard, int* move, const int color, const int depth, const int remain_depth,double alpha, double beta,tuple<int,int>* delta, int my_extra_moves, int oppo_extra_moves,int first_eat_bonus, int last_move){
	assert(alpha < beta);
	vector<Move2Strength> Moves;
	if (remain_depth > 0) {
		Expand(chessboard.Board, color, &Moves,-1);
		if (depth % 2 == 0){ // my move
			my_extra_moves = Moves.size();
		} else {
			oppo_extra_moves = Moves.size();
		}
		
	}
	else {// Quiescent
		//int last_eaten_piece = -get<0>(*delta);
		//if (last_eaten_piece >= 0)
		Expand(chessboard.Board, color, &Moves, last_move, true);

	}
	sort(Moves.begin(), Moves.end(), [](Move2Strength a, Move2Strength b) { return a.evaluator < b.evaluator; });

	//bool isQ = isQuiescent(&Moves);
	if(isTimeUp() || // time is up
		//(remain_depth == 0 and isQ)|| // reach limit of depth
		chessboard.Red_Chess_Num == 0 || // terminal node (no chess type)
		chessboard.Black_Chess_Num == 0 || // terminal node (no chess type)
		Moves.empty() // terminal node (no move type)
		){
		this->node++;
		// odd: *-1, even: *1
		int isend = remain_depth <= 0?min(chessboard.Red_Chess_Num ,chessboard.Black_Chess_Num ):Moves.size();
		double e = Evaluate(&chessboard, isend, color,my_extra_moves,oppo_extra_moves,first_eat_bonus) * (depth&1 ? -1 : 1);
		// if (depth + remain_depth == 8){
		// 	fprintf(stderr, "reward: %+1.5lf",e); //DEBUG
		// 	exit(0); // DEBUG
		// }
		//fprintf(stderr, "reward: %+1.5lf",e); //DEBUG
		//fflush(stderr); //DEBUG
		// if (GLOBALTURN == 100) {
		// 	fprintf(stderr, "First eat bonus: %d\n",first_eat_bonus); //DEBUG
			//fflush(stderr); //DEBUG
			//exit(0); // DEBUG
		// }
		return e;
	}
	double m = alpha-1;//-DBL_MAX;
	int new_move;
	// search deeper
	double t = alpha;
	int best_eat = 3;
	for (vector<Move2Strength>::iterator it = Moves.begin();it != Moves.end(); ++it){ // move
		
		
		int thismove = it -> move;
		auto thisevaluator = it -> evaluator;
		int negthiseat = get<0>(thisevaluator);
		// if (best_eat > 0 && remain_depth <= 0)
		// 	best_eat = negthiseat;
		// else if (remain_depth <= 0){
		// 	if (negthiseat > best_eat)
		// 		break;
		// }
		if (get<0>(thisevaluator) <=0)
			first_eat_bonus += (20-depth) * ((depth % 2 == 0)?1:-1);
		// if (remain_depth < 0 && thisevaluator > *delta) // Only search Quiscent
		// 	break;
		ChessBoard new_chessboard = chessboard;
		MakeMove(&new_chessboard, thismove, 0); // 0: dummy

		// if (depth + remain_depth == 8){
		// 	int StartPoint = thismove/100;  //DEBUG
		// 	int EndPoint = thismove%100;  //DEBUG
		// 	char moves[6];
		// 	sprintf(moves, "%c%c-%c%c",'a'+(StartPoint%4),'1'+(7-StartPoint/4),'a'+(EndPoint%4),'1'+(7-EndPoint/4));  //DEBUG
		// 	fprintf(stderr, "Depth: %2d, alpha: %+1.5lf, beta: %+1.5lf, Move: %s\n",depth, alpha, beta, moves); //DEBUG
		// 	fflush(stderr); //DEBUG
		// }
		

		double val = -1;
		if(new_chessboard.NoEatFlip >= NOEATFLIP_LIMIT){ // check no eat
			val = - (DRAW-WIN);
		} 
		// else if (isDraw(&new_chessboard)){
		// 	val = -(DRAW-WIN);
		// }
		// else if(hasAppeared(&new_chessboard)){
		// 	if (color == this->Color)
		// 		val = +(DRAW-WIN);
		// 	else
		// 		val = -(DRAW-WIN);
		// } 
		else{
			val = -Nega_max(new_chessboard, &new_move, color^1, depth+1, remain_depth-1, -1*beta, -1*alpha,&thisevaluator,my_extra_moves,oppo_extra_moves,first_eat_bonus,thismove);

		}
		if (get<0>(thisevaluator) <=0)
			first_eat_bonus -= (20-depth) * ((depth % 2 == 0)?1:-1);
		
		t = max(t,val);
		
		// if (depth == 10)
		// 	exit(0);
		alpha = max(alpha, t);
		
		if(t > m){ 
			m = t;
			*move = thismove;
		}
		if (alpha >= beta){
			break;
		}
		
		
	}
	return m;
	
}
bool MyAI::hasAppeared(const ChessBoard* chessboard){
	// Position Repetition
	int last_idx = chessboard->HistoryCount - 1;
	// -2: my previous ply
	int idx = last_idx - 2;
	// All ply must be move type
	int smallest_identical_idx = last_idx - (chessboard->NoEatFlip);
	// check loop
	while(idx >= 0 && idx >= smallest_identical_idx){
		if(chessboard->History[idx] == chessboard->History[last_idx])
			return true;
		idx -= 2;
	}
	return false;
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

bool MyAI::isTimeUp(){
	this->timeIsUp = ((double)(clock() - begin) / CLOCKS_PER_SEC  >= TIME_LIMIT);
	return this->timeIsUp;
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


