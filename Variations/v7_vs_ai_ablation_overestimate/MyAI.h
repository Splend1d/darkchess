#ifndef MYAI_INCLUDED
#define MYAI_INCLUDED 

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <stdint.h>
#include <set>
#include <vector>
#include <tuple>
#include <utility>

using namespace std;


#define RED 0
#define BLACK 1
#define CHESS_COVER -1
#define CHESS_EMPTY -2
#define COMMAND_NUM 19

struct ChessBoard{
	int Board[32];
	int CoverChess[14];
	int Red_Chess_Num, Black_Chess_Num;
	int NoEatFlip;
	int History[4096];
	int HistoryCount;

	ChessBoard(){}
	ChessBoard(const ChessBoard &chessBoard){
		memcpy(this->Board, chessBoard.Board, 32*sizeof(int));
		memcpy(this->CoverChess, chessBoard.CoverChess, 14*sizeof(int));
		this->Red_Chess_Num = chessBoard.Red_Chess_Num;
		this->Black_Chess_Num = chessBoard.Black_Chess_Num;
		this->NoEatFlip = chessBoard.NoEatFlip;
		memcpy(this->History, chessBoard.History, chessBoard.HistoryCount*sizeof(int));
		this->HistoryCount = chessBoard.HistoryCount;
	}
};
struct Move2Strength
{
    int move;
    tuple<int,int,int> evaluator;

};

class MyAI  
{
	const char* commands_name[COMMAND_NUM] = {
		"protocol_version",
		"name",
		"version",
		"known_command",
		"list_commands",
		"quit",
		"boardsize",
		"reset_board",
		"num_repetition",
		"num_moves_to_draw",
		"move",
		"flip",
		"genmove",
		"game_over",
		"ready",
		"time_settings",
		"time_left",
  	"showboard",
		"init_board"
	};
public:
	MyAI(void);
	~MyAI(void);

	// commands
	bool protocol_version(const char* data[], char* response);// 0
	bool name(const char* data[], char* response);// 1
	bool version(const char* data[], char* response);// 2
	bool known_command(const char* data[], char* response);// 3
	bool list_commands(const char* data[], char* response);// 4
	bool quit(const char* data[], char* response);// 5
	bool boardsize(const char* data[], char* response);// 6
	bool reset_board(const char* data[], char* response);// 7
	bool num_repetition(const char* data[], char* response);// 8
	bool num_moves_to_draw(const char* data[], char* response);// 9
	bool move(const char* data[], char* response);// 10
	bool flip(const char* data[], char* response);// 11
	bool genmove(const char* data[], char* response);// 12
	bool game_over(const char* data[], char* response);// 13
	bool ready(const char* data[], char* response);// 14
	bool time_settings(const char* data[], char* response);// 15
	bool time_left(const char* data[], char* response);// 16
	bool showboard(const char* data[], char* response);// 17
	bool init_board(const char* data[], char* response);// 18




private:
	int Color;
	int Red_Time, Black_Time;
	ChessBoard main_chessboard;
	bool timeIsUp;


#ifdef WINDOWS
	clock_t begin;
#else
	struct timeval begin;
#endif

	// statistics
	int node;

	// Utils
	int GetFin(char c);
	int ConvertChessNo(int input);

	// Board
	void initBoardState();
	void initBoardState(const char* data[]);
	void generateMove(char move[6]);
	int MakeMoveAndReturn(ChessBoard* chessboard, const int move, const int chess);
	void MakeMove(ChessBoard* chessboard, const int move, const int chess);
	void MakeMove(ChessBoard* chessboard, const char move[6]);
	bool Referee(const int* board, const int Startoint, const int EndPoint, const int color);
	void Expand(const int* board, const int color, vector<Move2Strength>* Result, int last_eaten_pos);
	double Evaluate(const ChessBoard* chessboard, const int legal_move_count, const int color, int first_eat_bonus, int depth);
	double EvaluateLight(const ChessBoard* chessboard, const int legal_move_count, const int color, int first_eat_bonus, int depth);

	double NegaScout(const ChessBoard chessboard, int* move, const int color, const int depth, const int remain_depth, double alpha, double beta, int first_eat_bonus, int acc_skips,int last_eaten_pos);

	bool isDraw(const ChessBoard* chessboard);
	bool isFinish(const ChessBoard* chessboard, int move_count);
	long get_relavant_positions(const ChessBoard chessboard, const int position);

	double ElapsedTime();
	bool isTimeUp();

	// Display
	void Pirnf_Chess(int chess_no,char *Result);
	void Pirnf_Chessboard();
	
};

#endif

