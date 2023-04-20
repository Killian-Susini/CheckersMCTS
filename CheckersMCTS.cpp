#include <iostream>
#include <array>
#include <vector>
#include <cmath>
#include <random>
#include <chrono>
#include <queue>
#include <algorithm>
#include <unordered_map>
#include <bitset>
#include <chrono>
#include <omp.h>

#include <fstream>
#include <format>


#define MAX_THREADS 16

using namespace std;

const size_t MAX_LEGAL_MOVE = 32;
const size_t MAX_CODE_MOVE = 8*4*8*4*4; // does not count if we eat many people

enum class SquareType
{
	EMPTY,
	BLACK_MAN,
	WHITE_MAN,
	BLACK_KING,
	WHITE_KING,
};
using enum SquareType;

random_device rd;
uniform_int_distribution int_distribution(0,65534);


vector<vector<vector<size_t>>> hashTable{};

int hashTurn;





class Move
{
public:
	size_t x1, x2, y1, y2;
	SquareType color;
	vector < pair < size_t, size_t >> piecesToRemove;
	
	inline void extend_move(size_t new_x2, size_t new_y2, pair<size_t, size_t> _piece_to_remove) {
		x2 = new_x2;
		y2 = new_y2;
		piecesToRemove.push_back(_piece_to_remove);
	}

	inline bool is_eating(size_t x, size_t y) {
		return find(piecesToRemove.begin(), piecesToRemove.end(), pair<size_t,size_t>{x,y}) != piecesToRemove.end();
	}
	
	Move(size_t _x1, size_t _y1, size_t _x2, size_t _y2, SquareType _color) : x1(_x1), y1(_y1), x2(_x2), y2(_y2), color(_color)
	{
	}
	Move(size_t _x1, size_t _y1, size_t _x2, size_t _y2, SquareType _color, pair<size_t, size_t> _piece_to_remove) : x1(_x1), y1(_y1), x2(_x2), y2(_y2), color(_color)
	{
		piecesToRemove.push_back(_piece_to_remove);
	}


	Move(size_t _x1, size_t _y1, size_t _x2, size_t _y2, SquareType _color, const vector<pair<size_t,size_t>>& _piecesToRemove) : x1(_x1), y1(_y1), x2(_x2), y2(_y2), color(_color), piecesToRemove(_piecesToRemove)
	{
	}
	Move(size_t _x1, size_t _y1, size_t _x2, size_t _y2, SquareType _color, vector<pair<size_t, size_t>>&& _piecesToRemove) : x1(_x1), y1(_y1), x2(_x2), y2(_y2), color(_color), piecesToRemove(_piecesToRemove)
	{
	}

	size_t code() {
		switch (color)
		{
		case BLACK_MAN:
			return x1 + 4 * (y1 >> 1) + 8 * 4 * x2 + 4 * 8 * 4 * (y2 >> 1);
			break;
		case WHITE_MAN:
			return x1 + 4 * (y1 >> 1) + 8 * 4 * x2 + 4 * 8 * 4 * (y2 >> 1) + (8 * 4 * 8 * 4);
			break;
		case BLACK_KING:
			return x1 + 4 * (y1 >> 1) + 8 * 4 * x2 + 4 * 8 * 4 * (y2 >> 1) + 2*(8 * 4 * 8 * 4);
			break;
		case WHITE_KING:
			return x1 + 4 * (y1 >> 1) + 8 * 4 * x2 + 4 * 8 * 4 * (y2 >> 1) + 3*(8 * 4 * 8 * 4);
			break;
		default:
			break;
		}
	}
private:

};

class TranspoMonteCarlo
{
public:
	double n = 0.0;
	array<double, MAX_LEGAL_MOVE> nplayouts = {};
	array<double, MAX_LEGAL_MOVE> nwins = {};
	unordered_map<size_t, double> nplayouts_AMAF = {};
	unordered_map<size_t, double> nwins_AMAF = {};
private:

};

class CheckerBoard
{

public:
	size_t h;
	bool white_turn;
	array<array<SquareType, 8>, 8> board;
	size_t turn_since_last_capture_or_pawn_move = 0;
	vector<Move> legal_moves() const {
		vector<Move> moves;
		if (white_turn) {
			vector<pair<size_t, size_t>> white_man;
			vector<pair<size_t, size_t>> white_king;

			for (size_t i = 0; i < 8; i++) {
				for (size_t j = 0; j < 8; j++) {
					if (board[i][j] == WHITE_MAN) white_man.push_back({ i,j });
					else if (board[i][j] == WHITE_KING) white_king.push_back({ i,j });
				}
			}

			for (const auto& white_m : white_man) {
				auto x1 = white_m.first;
				auto y1 = white_m.second;

				vector<Move> moves_to_add;
				size_t curr_move_index = 0;
				if (x1 > 1 && y1 > 1 && (board[x1 - 1][y1 - 1] == BLACK_MAN || board[x1 - 1][y1 - 1] == BLACK_KING) && board[x1 - 2][y1 - 2] == EMPTY) {
					moves_to_add.push_back(Move(x1, y1, x1 - 2, y1 - 2, WHITE_MAN, pair<size_t, size_t>{x1 - 1, y1 - 1}));
				}
				if (x1 > 1 && y1 < 6 && (board[x1 - 1][y1 + 1] == BLACK_MAN || board[x1 - 1][y1 + 1] == BLACK_KING) && board[x1 - 2][y1 + 2] == EMPTY) {
					moves_to_add.push_back(Move(x1, y1, x1 - 2, y1 + 2, WHITE_MAN, pair<size_t, size_t>{x1 - 1, y1 + 1}));
				}

				while (not (curr_move_index == moves_to_add.size()))
				{
					Move move = moves_to_add[curr_move_index];
					auto x = move.x2;
					auto y = move.y2;

					if (x > 1 && y > 1 && (board[x - 1][y - 1] == BLACK_MAN || board[x - 1][y - 1] == BLACK_KING) && board[x - 2][y - 2] == EMPTY) {
						if (x > 1 && y < 6 && (board[x - 1][y + 1] == BLACK_MAN || board[x - 1][y + 1] == BLACK_KING) && board[x - 2][y + 2] == EMPTY) {
							Move new_move = move;
							new_move.extend_move(x - 2, y + 2, { x - 1,y + 1 });
							moves_to_add.push_back(new_move); // need to consider the new move later, as well as the new currently considered move
						}
						moves_to_add[curr_move_index].extend_move(x - 2, y - 2, { x - 1,y - 1 });
						
					} else if (x > 1 && y < 6 && (board[x - 1][y + 1] == BLACK_MAN || board[x - 1][y + 1] == BLACK_KING) && board[x - 2][y + 2] == EMPTY) {
						moves_to_add[curr_move_index].extend_move(x - 2, y + 2, { x - 1,y + 1 });
					}
					else {
						// no possible extension of this move, increment the move counter
						curr_move_index++;
					}
				}
				moves.insert(moves.end(), moves_to_add.begin(), moves_to_add.end());
			}

			for (const auto& white_k : white_king) {
				auto x1 = white_k.first;
				auto y1 = white_k.second;

				vector<Move> moves_to_add;
				size_t curr_move_index = 0;
				if (x1 > 1 && y1 > 1 && (board[x1 - 1][y1 - 1] == BLACK_MAN || board[x1 - 1][y1 - 1] == BLACK_KING) && board[x1 - 2][y1 - 2] == EMPTY) {
					moves_to_add.push_back(Move(x1, y1, x1 - 2, y1 - 2, WHITE_KING, pair<size_t, size_t>{x1 - 1, y1 - 1}));
				}
				if (x1 > 1 && y1 < 6 && (board[x1 - 1][y1 + 1] == BLACK_MAN || board[x1 - 1][y1 + 1] == BLACK_KING) && board[x1 - 2][y1 + 2] == EMPTY) {
					moves_to_add.push_back(Move(x1, y1, x1 - 2, y1 + 2, WHITE_KING, pair<size_t, size_t>{x1 - 1, y1 + 1}));
				}
				if (x1 < 6 && y1 > 1 && (board[x1 + 1][y1 - 1] == BLACK_MAN || board[x1 + 1][y1 - 1] == BLACK_KING) && board[x1 + 2][y1 - 2] == EMPTY) {
					moves_to_add.push_back(Move(x1, y1, x1 + 2, y1 - 2, WHITE_KING, pair<size_t, size_t>{x1 + 1, y1 - 1}));
				}
				if (x1 < 6 && y1 < 6 && (board[x1 + 1][y1 + 1] == BLACK_MAN || board[x1 + 1][y1 + 1] == BLACK_KING) && board[x1 + 2][y1 + 2] == EMPTY) {
					moves_to_add.push_back(Move(x1, y1, x1 + 2, y1 + 2, WHITE_KING, pair<size_t, size_t>{x1 + 1, y1 + 1}));
				}


				while (not (curr_move_index == moves_to_add.size()))
				{
					Move move = moves_to_add[curr_move_index];
					auto x = move.x2;
					auto y = move.y2;

					if (x > 1 && y > 1 && !move.is_eating(x - 1, y - 1) && (board[x - 1][y - 1] == BLACK_MAN || board[x - 1][y - 1] == BLACK_KING) && board[x - 2][y - 2] == EMPTY) {
						if (x > 1 && y < 6 && !move.is_eating(x - 1, y + 1) && (board[x - 1][y + 1] == BLACK_MAN || board[x - 1][y + 1] == BLACK_KING) && board[x - 2][y + 2] == EMPTY) {
							Move new_move = move;
							new_move.extend_move(x - 2, y + 2, { x - 1,y + 1 });
							moves_to_add.push_back(new_move); // need to consider the new move later, as well as the new currently considered move
						}
						if (x < 6 && y > 1 && !move.is_eating(x + 1, y - 1) && (board[x + 1][y - 1] == BLACK_MAN || board[x + 1][y - 1] == BLACK_KING) && board[x + 2][y - 2] == EMPTY) {
							Move new_move = move;
							new_move.extend_move(x + 2, y - 2, { x + 1,y - 1 });
							moves_to_add.push_back(new_move); // need to consider the new move later, as well as the new currently considered move
						}
						if (x < 6 && y < 6 && !move.is_eating(x + 1, y + 1) && (board[x + 1][y + 1] == BLACK_MAN || board[x + 1][y + 1] == BLACK_KING) && board[x + 2][y + 2] == EMPTY) {
							Move new_move = move;
							new_move.extend_move(x + 2, y + 2, { x + 1,y + 1 });
							moves_to_add.push_back(new_move); // need to consider the new move later, as well as the new currently considered move
						}
						moves_to_add[curr_move_index].extend_move(x - 2, y - 2, { x - 1,y - 1 });
					}
					else if (x > 1 && y < 6 && !move.is_eating(x - 1, y + 1) && (board[x - 1][y + 1] == BLACK_MAN || board[x - 1][y + 1] == BLACK_KING) && board[x - 2][y + 2] == EMPTY) {
						if (x < 6 && y > 1 && !move.is_eating(x + 1, y - 1) && (board[x + 1][y - 1] == BLACK_MAN || board[x + 1][y - 1] == BLACK_KING) && board[x + 2][y - 2] == EMPTY) {
							Move new_move = move;
							new_move.extend_move(x + 2, y - 2, { x + 1,y - 1 });
							moves_to_add.push_back(new_move); // need to consider the new move later, as well as the new currently considered move
						}
						if (x < 6 && y < 6 && !move.is_eating(x + 1, y + 1) && (board[x + 1][y + 1] == BLACK_MAN || board[x + 1][y + 1] == BLACK_KING) && board[x + 2][y + 2] == EMPTY) {
							Move new_move = move;
							new_move.extend_move(x + 2, y + 2, { x + 1,y + 1 });
							moves_to_add.push_back(new_move); // need to consider the new move later, as well as the new currently considered move
						}
						moves_to_add[curr_move_index].extend_move(x - 2, y + 2, { x - 1, y + 1 });
					}
					else if (x < 6 && y > 1 && !move.is_eating(x + 1, y - 1) && (board[x + 1][y - 1] == BLACK_MAN || board[x + 1][y - 1] == BLACK_KING) && board[x + 2][y - 2] == EMPTY) {
						if (x < 6 && y < 6 && !move.is_eating(x + 1, y + 1) && (board[x + 1][y + 1] == BLACK_MAN || board[x + 1][y + 1] == BLACK_KING) && board[x + 2][y + 2] == EMPTY) {
							Move new_move = move;
							new_move.extend_move(x + 2, y + 2, { x + 1,y + 1 });
							moves_to_add.push_back(new_move); // need to consider the new move later, as well as the new currently considered move
						}
						moves_to_add[curr_move_index].extend_move(x + 2, y - 2, { x + 1, y - 1 });
					}
					else if (x < 6 && y < 6 && !move.is_eating(x + 1, y + 1) && (board[x + 1][y + 1] == BLACK_MAN || board[x + 1][y + 1] == BLACK_KING) && board[x + 2][y + 2] == EMPTY) {
						moves_to_add[curr_move_index].extend_move(x + 2, y + 2, { x + 1,y + 1 });
					}
					else {
						// no possible extension of this move, increment the move counter
						curr_move_index++;
					}
				}
				moves.insert(moves.end(), moves_to_add.begin(), moves_to_add.end());
			}

			if (moves.empty()) { // if no eat actions are possible, add 'normal' moves
				for (const auto& white_m : white_man) {
					auto x1 = white_m.first;
					auto y1 = white_m.second;
					if (x1 > 0 && y1 > 0 && board[x1 - 1][y1 - 1] == EMPTY) {
						moves.push_back(Move(x1, y1, x1 - 1, y1 - 1, WHITE_MAN));
					}
					if (x1 > 0 && y1 < 7 && board[x1 - 1][y1 + 1] == EMPTY) {
						moves.push_back(Move(x1, y1, x1 - 1, y1 + 1, WHITE_MAN));
					}

				}

				for (const auto& white_k : white_king) {
					auto x1 = white_k.first;
					auto y1 = white_k.second;
					if (x1 > 0 && y1 > 0 && board[x1 - 1][y1 - 1] == EMPTY) {
						moves.push_back(Move(x1, y1, x1 - 1, y1 - 1, WHITE_KING));
					}
					if (x1 > 0 && y1 < 7 && board[x1 - 1][y1 + 1] == EMPTY) {
						moves.push_back(Move(x1, y1, x1 - 1, y1 + 1, WHITE_KING));
					}
					if (x1 < 7 && y1 > 0 && board[x1 + 1][y1 - 1] == EMPTY) {
						moves.push_back(Move(x1, y1, x1 + 1, y1 - 1, WHITE_KING));
					}
					if (x1 < 7 && y1 < 7 && board[x1 + 1][y1 + 1] == EMPTY) {
						moves.push_back(Move(x1, y1, x1 + 1, y1 + 1, WHITE_KING));
					}
				}
			}

		} else {
			vector<pair<size_t, size_t>> black_man;
			vector<pair<size_t, size_t>> black_king;


			for (size_t i = 0; i < 8; i++) {
				for (size_t j = 0; j < 8; j++) {
					if (board[i][j] == BLACK_MAN) black_man.push_back({ i,j });
					else if (board[i][j] == BLACK_KING) black_king.push_back({ i,j });
				}
			}

			for (const auto& black_m : black_man) {
				auto x1 = black_m.first;
				auto y1 = black_m.second;

				vector<Move> moves_to_add;
				size_t curr_move_index = 0;
				if (x1 < 6 && y1 > 1 && (board[x1 + 1][y1 - 1] == WHITE_MAN || board[x1 + 1][y1 - 1] == WHITE_KING) && board[x1 + 2][y1 - 2] == EMPTY) {
					moves_to_add.push_back(Move(x1, y1, x1 + 2, y1 - 2, BLACK_MAN, pair<size_t, size_t>{x1 + 1, y1 - 1}));
				}
				if (x1 < 6 && y1 < 6 && (board[x1 + 1][y1 + 1] == WHITE_MAN || board[x1 + 1][y1 + 1] == WHITE_KING) && board[x1 + 2][y1 + 2] == EMPTY) {
					moves_to_add.push_back(Move(x1, y1, x1 + 2, y1 + 2, BLACK_MAN, pair<size_t, size_t>{x1 + 1, y1 + 1}));
				}

				while (not (curr_move_index == moves_to_add.size()))
				{
					auto& move = moves_to_add[curr_move_index];
					auto x = move.x2;
					auto y = move.y2;

					if (x < 6 && y > 1 && (board[x + 1][y - 1] == WHITE_MAN || board[x + 1][y - 1] == WHITE_KING) && board[x + 2][y - 2] == EMPTY) {
						if (x < 6 && y < 6 && (board[x + 1][y + 1] == WHITE_MAN || board[x + 1][y + 1] == WHITE_KING) && board[x + 2][y + 2] == EMPTY) {
							Move new_move = move;
							new_move.extend_move(x + 2, y + 2, { x + 1,y + 1 });
							moves_to_add.push_back(new_move); // need to consider the new move later, as well as the new currently considered move
						}
						moves_to_add[curr_move_index].extend_move(x + 2, y - 2, { x + 1,y - 1 });

					}
					else if (x < 6 && y < 6 && (board[x + 1][y + 1] == WHITE_MAN || board[x + 1][y + 1] == WHITE_KING) && board[x + 2][y + 2] == EMPTY) {
						moves_to_add[curr_move_index].extend_move(x + 2, y + 2, { x + 1,y + 1 });
					}
					else {
						// no possible extension of this move, increment the move counter
						curr_move_index++;
					}
				}
				moves.insert(moves.end(), moves_to_add.begin(), moves_to_add.end());
			}

			for (const auto& black_k : black_king) {
				auto x1 = black_k.first;
				auto y1 = black_k.second;

				vector<Move> moves_to_add;
				size_t curr_move_index = 0;
				if (x1 > 1 && y1 > 1 && (board[x1 - 1][y1 - 1] == WHITE_MAN || board[x1 - 1][y1 - 1] == WHITE_KING) && board[x1 - 2][y1 - 2] == EMPTY) {
					moves_to_add.push_back(Move(x1, y1, x1 - 2, y1 - 2, BLACK_KING, pair<size_t, size_t>{x1 - 1, y1 - 1}));
				}
				if (x1 > 1 && y1 < 6 && (board[x1 - 1][y1 + 1] == WHITE_MAN || board[x1 - 1][y1 + 1] == WHITE_KING) && board[x1 - 2][y1 + 2] == EMPTY) {
					moves_to_add.push_back(Move(x1, y1, x1 - 2, y1 + 2, BLACK_KING, pair<size_t, size_t>{x1 - 1, y1 + 1}));
				}
				if (x1 < 6 && y1 > 1 && (board[x1 + 1][y1 - 1] == WHITE_MAN || board[x1 + 1][y1 - 1] == WHITE_KING) && board[x1 + 2][y1 - 2] == EMPTY) {
					moves_to_add.push_back(Move(x1, y1, x1 + 2, y1 - 2, BLACK_KING, pair<size_t, size_t>{x1 + 1, y1 - 1}));
				}
				if (x1 < 6 && y1 < 6 && (board[x1 + 1][y1 + 1] == WHITE_MAN || board[x1 + 1][y1 + 1] == WHITE_KING) && board[x1 + 2][y1 + 2] == EMPTY) {
					moves_to_add.push_back(Move(x1, y1, x1 + 2, y1 + 2, BLACK_KING, pair<size_t, size_t>{x1 + 1, y1 + 1}));
				}


				while (not (curr_move_index == moves_to_add.size()))
				{
					Move move = moves_to_add[curr_move_index];
					auto x = move.x2;
					auto y = move.y2;

					if (x > 1 && y > 1 && !move.is_eating(x - 1, y - 1) && (board[x - 1][y - 1] == WHITE_MAN || board[x - 1][y - 1] == WHITE_KING) && board[x - 2][y - 2] == EMPTY) {
						if (x > 1 && y < 6 && !move.is_eating(x - 1, y + 1) && (board[x - 1][y + 1] == WHITE_MAN || board[x - 1][y + 1] == WHITE_KING) && board[x - 2][y + 2] == EMPTY) {
							Move new_move = move;
							new_move.extend_move(x - 2, y + 2, { x - 1,y + 1 });
							moves_to_add.push_back(new_move); // need to consider the new move later, as well as the new currently considered move
						}
						if (x < 6 && y > 1 && !move.is_eating(x + 1, y - 1) && (board[x + 1][y - 1] == WHITE_MAN || board[x + 1][y - 1] == WHITE_KING) && board[x + 2][y - 2] == EMPTY) {
							Move new_move = move;
							new_move.extend_move(x + 2, y - 2, { x + 1,y - 1 });
							moves_to_add.push_back(new_move); // need to consider the new move later, as well as the new currently considered move
						}
						if (x < 6 && y < 6 && !move.is_eating(x + 1, y + 1) && (board[x + 1][y + 1] == WHITE_MAN || board[x + 1][y + 1] == WHITE_KING) && board[x + 2][y + 2] == EMPTY) {
							Move new_move = move;
							new_move.extend_move(x + 2, y + 2, { x + 1,y + 1 });
							moves_to_add.push_back(new_move); // need to consider the new move later, as well as the new currently considered move
						}
						moves_to_add[curr_move_index].extend_move(x - 2, y - 2, { x - 1,y - 1 });
					}
					else if (x > 1 && y < 6 && !move.is_eating(x - 1, y + 1) && (board[x - 1][y + 1] == WHITE_MAN || board[x - 1][y + 1] == WHITE_KING) && board[x - 2][y + 2] == EMPTY) {
						if (x < 6 && y > 1 && !move.is_eating(x + 1, y - 1) && (board[x + 1][y - 1] == WHITE_MAN || board[x + 1][y - 1] == WHITE_KING) && board[x + 2][y - 2] == EMPTY) {
							Move new_move = move;
							new_move.extend_move(x + 2, y - 2, { x + 1,y - 1 });
							moves_to_add.push_back(new_move); // need to consider the new move later, as well as the new currently considered move
						}
						if (x < 6 && y < 6 && !move.is_eating(x + 1, y + 1) && (board[x + 1][y + 1] == WHITE_MAN || board[x + 1][y + 1] == WHITE_KING) && board[x + 2][y + 2] == EMPTY) {
							Move new_move = move;
							new_move.extend_move(x + 2, y + 2, { x + 1,y + 1 });
							moves_to_add.push_back(new_move); // need to consider the new move later, as well as the new currently considered move
						}
						moves_to_add[curr_move_index].extend_move(x - 2, y + 2, { x - 1, y + 1 });
					}
					else if (x < 6 && y > 1 && !move.is_eating(x + 1, y - 1) && (board[x + 1][y - 1] == WHITE_MAN || board[x + 1][y - 1] == WHITE_KING) && board[x + 2][y - 2] == EMPTY) {
						if (x < 6 && y < 6 && !move.is_eating(x + 1, y + 1) && (board[x + 1][y + 1] == WHITE_MAN || board[x + 1][y + 1] == WHITE_KING) && board[x + 2][y + 2] == EMPTY) {
							Move new_move = move;
							new_move.extend_move(x + 2, y + 2, { x + 1,y + 1 });
							moves_to_add.push_back(new_move); // need to consider the new move later, as well as the new currently considered move
						}
						moves_to_add[curr_move_index].extend_move(x + 2, y - 2, { x + 1, y - 1 });
					}
					else if (x < 6 && y < 6 && !move.is_eating(x + 1, y + 1) && (board[x + 1][y + 1] == WHITE_MAN || board[x + 1][y + 1] == WHITE_KING) && board[x + 2][y + 2] == EMPTY) {
						moves_to_add[curr_move_index].extend_move(x + 2, y + 2, { x + 1,y + 1 });
					}
					else {
						// no possible extension of this move, increment the move counter
						curr_move_index++;
					}
				}
				moves.insert(moves.end(), moves_to_add.begin(), moves_to_add.end());
			}
			if (moves.empty()) { // if no eat actions are possible, add 'normal' moves
				for (const auto& black_m : black_man) {
					auto x1 = black_m.first;
					auto y1 = black_m.second;
					if (x1 < 7 && y1 > 0 && board[x1 + 1][y1 - 1] == EMPTY) {
						moves.push_back(Move(x1, y1, x1 + 1, y1 - 1, BLACK_MAN));
					}
					if (x1 < 7 && y1 < 7 && board[x1 + 1][y1 + 1] == EMPTY) {
						moves.push_back(Move(x1, y1, x1 + 1, y1 + 1, BLACK_MAN));
					}
				}
				for (const auto& black_k : black_king) {
					auto x1 = black_k.first;
					auto y1 = black_k.second;
					if (x1 > 0 && y1 > 0 && board[x1 - 1][y1 - 1] == EMPTY) {
						moves.push_back(Move(x1, y1, x1 - 1, y1 - 1, BLACK_KING));
					}
					if (x1 > 0 && y1 < 7 && board[x1 - 1][y1 + 1] == EMPTY) {
						moves.push_back(Move(x1, y1, x1 - 1, y1 + 1, BLACK_KING));
					}
					if (x1 < 7 && y1 > 0 && board[x1 + 1][y1 - 1] == EMPTY) {
						moves.push_back(Move(x1, y1, x1 + 1, y1 - 1, BLACK_KING));
					}
					if (x1 < 7 && y1 < 7 && board[x1 + 1][y1 + 1] == EMPTY) {
						moves.push_back(Move(x1, y1, x1 + 1, y1 + 1, BLACK_KING));
					}
				}
			}
		}
		return moves;
	}

	bool terminal() const { //  check repeats?
		if (turn_since_last_capture_or_pawn_move >= 25) return true;
		if (legal_moves().size() == 0) return true; // this takes care of cases of no move and loss of all pieces
		return false;
	}
	
	double score() const {
		if (terminal()) {
			if (turn_since_last_capture_or_pawn_move >= 25) return 0.5;
			// simplification, we should technically check if we ourselves can make moves: in that case it is a draw
			if (white_turn) return 0.0;
			return 1.0;
		}
		else return 0.5;
	}

	void play(const Move& move) {
		board[move.x1][move.y1] = EMPTY;
		for (auto& pos: move.piecesToRemove)
		{
			board[pos.first][pos.second] = EMPTY;
		}
		if (move.color == BLACK_MAN || move.color == WHITE_MAN || move.piecesToRemove.size() > 0)
			turn_since_last_capture_or_pawn_move = 0;
		else
			turn_since_last_capture_or_pawn_move += 1;
		if (white_turn && move.x2 == 0) {
			board[move.x2][move.y2] = WHITE_KING;
			h ^= hashTable[move.x1][move.y1][3];
			h ^= hashTable[move.x2][move.y2][3];
		}
		else if (!white_turn && move.x2 == 7) {
			board[move.x2][move.y2] = BLACK_KING;
			h ^= hashTable[move.x1][move.y1][2];
			h ^= hashTable[move.x2][move.y2][2];
		}
		else {
			board[move.x2][move.y2] = move.color;
			switch (move.color)
			{
			case BLACK_MAN:
				h ^= hashTable[move.x1][move.y1][0];
				h ^= hashTable[move.x2][move.y2][0];
				break;
			case WHITE_MAN:
				h ^= hashTable[move.x1][move.y1][1];
				h ^= hashTable[move.x2][move.y2][1];
				break;
			case BLACK_KING:
				h ^= hashTable[move.x1][move.y1][2];
				h ^= hashTable[move.x2][move.y2][2];
				break;
			case WHITE_KING:
				h ^= hashTable[move.x1][move.y1][3];
				h ^= hashTable[move.x2][move.y2][3];
				break;
			default:
				break;
			}
		}
		white_turn = !white_turn;
		h ^= hashTurn;
	}

	double playout(default_random_engine& generator) {
		
		while (!terminal()) {
			auto moves = legal_moves();
			/*cout << "after legal_moves \n";
			cout << print_board();
			for (auto& move : moves)
			{
				auto to_print = format("({},{})->({},{})", move.x1, move.y1, move.x2, move.y2);
				cout << to_print;
				for (auto eat : move.piecesToRemove)
					cout << eat.first << " " << eat.second;
				
			}*/

			uniform_int_distribution<> distribution(0, moves.size()-1);
			auto selected = distribution(generator);
			//cout << selected << '\n';
			play(moves[selected]);
		}
		//cout << "end_of_playout ";
		return score();
	}
	

	double playoutAMAF(vector<size_t>& played, default_random_engine& generator) {

		while (!terminal()) {
			auto moves = legal_moves();
			/*cout << "after legal_moves \n";
			cout << print_board();
			for (auto& move : moves)
			{
				auto to_print = format("({},{})->({},{})", move.x1, move.y1, move.x2, move.y2);
				cout << to_print;
				for (auto eat : move.piecesToRemove)
					cout << eat.first << " " << eat.second;

			}*/

			uniform_int_distribution<> distribution(0, moves.size() - 1);
			auto selected = distribution(generator);
			//cout << selected << '\n';
			play(moves[selected]);
			played.push_back(moves[selected].code());
		}
		//cout << "end_of_playout ";
		return score();
	}

	string print_board() const {
		string ret;
		for (size_t i = 0; i < 8; i++)
		{
			for (size_t j = 0; j < 8; j++)
			{
				if (board[i][j] == EMPTY)
					ret.append(" ");
				else if (board[i][j] == BLACK_MAN)
					ret.append("M");
				else if (board[i][j] == BLACK_KING)
					ret.append("K");
				else if (board[i][j] == WHITE_MAN)
					ret.append("m");
				else if (board[i][j] == WHITE_KING)
					ret.append("k");
			}
			ret.append("\n");
		}
		return ret;
	}


	CheckerBoard()
	{
		white_turn = true;
		h = 0;
		h ^= hashTurn;
		board = {{{EMPTY,BLACK_MAN,EMPTY,BLACK_MAN,EMPTY,BLACK_MAN,EMPTY,BLACK_MAN},
				  {BLACK_MAN,EMPTY,BLACK_MAN,EMPTY,BLACK_MAN,EMPTY,BLACK_MAN,EMPTY},
			      {EMPTY,BLACK_MAN,EMPTY,BLACK_MAN,EMPTY,BLACK_MAN,EMPTY,BLACK_MAN},
				  {EMPTY,    EMPTY,EMPTY,    EMPTY,EMPTY,    EMPTY,EMPTY,    EMPTY},
				  {EMPTY,    EMPTY,EMPTY,    EMPTY,EMPTY,    EMPTY,EMPTY,    EMPTY},
				  {WHITE_MAN,EMPTY,WHITE_MAN,EMPTY,WHITE_MAN,EMPTY,WHITE_MAN,EMPTY},
				  {EMPTY,WHITE_MAN,EMPTY,WHITE_MAN,EMPTY,WHITE_MAN,EMPTY,WHITE_MAN},
				  {WHITE_MAN,EMPTY,WHITE_MAN,EMPTY,WHITE_MAN,EMPTY,WHITE_MAN,EMPTY}}};
		
		for (size_t i = 0; i < 8; i++) {
			for (size_t j = 0; j < 8; j++) {
				switch (board[i][j])
				{
				case BLACK_MAN:
					h ^= hashTable[i][j][0];
					break;
				case WHITE_MAN:
					h ^= hashTable[i][j][1];
					break;
				case BLACK_KING:
					h ^= hashTable[i][j][2];
					break;
				case WHITE_KING:
					h ^= hashTable[i][j][3];
					break;
				default:
					break;
				}
			}
		}

	}

};



class TableMonteCarlo
{
	vector<TranspoMonteCarlo> table;
	bitset<65536> exist;
public:
	TableMonteCarlo()
	{
		table.reserve(65536);
		for (size_t i = 0; i < 65536; i++)
		{
			table.push_back(TranspoMonteCarlo());
		}
		exist.reset();
	}

	bool look(const CheckerBoard& board) {
		return exist[board.h];
	}

	TranspoMonteCarlo& get(const CheckerBoard& board) {
		return table[board.h];
	}

	void add(const CheckerBoard& board) {
		table[board.h] = TranspoMonteCarlo();
		exist[board.h] = true;
	}

	void updateAMAF(TranspoMonteCarlo& t,const vector<size_t>& played, double res) {
		size_t len_played = played.size();
		bitset<MAX_CODE_MOVE> already_seen{};
		for (size_t i = 0; i < len_played; i++)
		{
			if (already_seen[played[i]])
				continue;
			else
				already_seen[played[i]] = true;
			if (t.nplayouts_AMAF.find(played[i]) != t.nplayouts_AMAF.end()) {
				t.nplayouts_AMAF[played[i]] += 1.0;
				t.nwins_AMAF[played[i]] += res;
			}
			else {
				t.nplayouts_AMAF[played[i]] = 1.0;
				t.nwins_AMAF[played[i]] = res;
			}
		}
	}
};


Move flat(const CheckerBoard& state, size_t n_playouts) {
	default_random_engine generator(rd());
	const auto moves = state.legal_moves();
	double bestScore = 0.0;
	size_t bestMove = 0;
	for (size_t i = 0; i < moves.size(); i++) {
		double sum = 0;
		const size_t num_playouts = n_playouts / moves.size();
		for (size_t j = 0; j < num_playouts; j++) {
			//cout << "move " << i << " playout " << j << "\n";
			CheckerBoard copy_state = state;
			copy_state.play(moves[i]);
			auto r = copy_state.playout(generator);
			if (!state.white_turn)
				r = 1 - r;
			sum += r;
		}
		if (sum > bestScore) {
			bestScore = sum;
			bestMove = i;
		}
	}
	//cout << "done\n";
	return moves[bestMove];
}
	

double UCT(CheckerBoard& state, TableMonteCarlo& table, default_random_engine& generator, double C=1.42) {
	if (state.terminal()) {
		return state.score();
	}
	if (table.look(state)) {
		auto& t = table.get(state);
		double bestValue = 0;
		size_t bestMove = 0;
		auto moves = state.legal_moves();
		size_t num_moves = moves.size();
		for (size_t i = 0; i < num_moves; i++)
		{
			double val = DBL_MAX;
			auto n = t.n;
			auto ni = t.nplayouts[i];
			auto wi = t.nwins[i];
			if (ni > 0) {
				double Q = wi/ ni;
				if (!state.white_turn)
					Q = 1 - Q;
				val = Q + C * sqrt(log(n) / ni);
			}
			if (val > bestValue) {
				bestValue = val;
				bestMove = i;
			}
		}
		state.play(moves[bestMove]);
		auto res = UCT(state, table, generator, C);
		t.n++;
		t.nplayouts[bestMove] += 1.0;
		
		t.nwins[bestMove] += res;
		return res;
	}
	else {
		table.add(state);
		return state.playout(generator);
	}
}

Move bestMoveUCT(const CheckerBoard& state, size_t n_playouts, double C = 1.42) {

	default_random_engine generator(rd());
	TableMonteCarlo table{};
	table.add(state);
	for (size_t i = 0; i < n_playouts; i++)
	{
		CheckerBoard copy_state = state;
		double res = UCT(copy_state, table, generator, C);
	}
	auto& t = table.get(state);
	auto moves = state.legal_moves();
	double bestValue = t.nwins[0]/t.nplayouts[0];
	if (!state.white_turn) bestValue = 1 - bestValue;
	size_t bestMove = 0;
	const size_t num_moves = moves.size();
	for (size_t i = 1; i < num_moves; i++)
	{
		double move_value = t.nwins[i] / t.nplayouts[i];
		if (!state.white_turn) move_value = 1 - move_value;
		if (move_value > bestValue) {
			bestValue = move_value;
			bestMove = i;
		}
	}
	return moves[bestMove];
}


double GRAVE(CheckerBoard& state, vector<size_t>& played, TableMonteCarlo& table, TranspoMonteCarlo& tref, default_random_engine& generator, size_t ref = 10, double b = 1e-5) {
	if (state.terminal()) {
		return state.score();
	}
	if (table.look(state)) {
		auto& t = table.get(state);
		TranspoMonteCarlo tr;
		if (t.n > ref)
			tr = t;
		else
			tr = tref;
			
		double bestValue = 0;
		auto moves = state.legal_moves();
		size_t bestMove = 0;
		size_t bestCode = moves[0].code();
		size_t num_moves = moves.size();
		for (size_t i = 0; i < num_moves; i++)
		{
			double val = DBL_MAX;
			auto code = moves[i].code();
			auto n = t.n;
			auto ni = t.nplayouts[i];
			auto wi = t.nwins[i];
			if (tr.nplayouts_AMAF.find(code) == tr.nplayouts_AMAF.end()) {
				tr.nplayouts_AMAF[code] = 0.0;
				tr.nwins_AMAF[code] = 0.0;
			}
			auto niAMAF = tr.nplayouts_AMAF[code];
			auto wiAMAF = tr.nwins_AMAF[code];

			double beta = 0.0;
			double Q_tilde = 1000000;
			if (niAMAF > 0) {
				beta = niAMAF / (ni + niAMAF + b * ni * niAMAF);
				double Q_tilde = wiAMAF / niAMAF;
				if (!state.white_turn)
					Q_tilde = 1 - Q_tilde;
			}

			double Q = 1000000;
			if (ni > 0) {
				Q = wi / ni;
				if (!state.white_turn)
					Q = 1 - Q;
				Q = Q + 0.4 * sqrt(log(n) / ni);
			}

			val = (1.0 - beta) * Q + beta * Q_tilde;
			if (val > bestValue) {
				bestValue = val;
				bestMove = i;
				bestCode = code;
			}
		}
		state.play(moves[bestMove]);
		played.push_back(bestCode);
		auto res = GRAVE(state,played, table,tr, generator);
		t.n++;
		t.nplayouts[bestMove] += 1.0;
		t.nwins[bestMove] += res;
		table.updateAMAF(t, played, res);
		return res;
	}
	else {
		table.add(state);
		return state.playoutAMAF(played, generator);
	}
}

Move bestMoveGRAVE(const CheckerBoard& state, size_t n_playouts) {

	default_random_engine generator(rd());
	TableMonteCarlo table{};
	table.add(state);
	auto& tref = table.get(state);
	for (size_t i = 0; i < n_playouts; i++)
	{
		CheckerBoard copy_state = state;
		vector<size_t> played{};
		double res = GRAVE(copy_state, played, table, tref, generator);
	}
	auto& t = table.get(state);
	auto moves = state.legal_moves();
	double bestValue = t.nwins[0] / t.nplayouts[0];
	if (!state.white_turn) bestValue = 1 - bestValue;
	size_t bestMove = 0;
	const size_t num_moves = moves.size();
	for (size_t i = 1; i < num_moves; i++)
	{
		double move_value = t.nwins[i] / t.nplayouts[i];
		if (!state.white_turn) move_value = 1 - move_value;
		if (move_value > bestValue) {
			bestValue = move_value;
			bestMove = i;
		}
	}
	return moves[bestMove];
}




//Move BestMoveSequentialHalving(const CheckerBoard& state, size_t n_playouts) {
//	TableMonteCarlo table{};
//	table.add(state);
//	auto moves = state.legal_moves();
//	const auto number_of_moves = moves.size();
//	vector<double> nplayouts(number_of_moves, 0);
//	vector<double> nwins(number_of_moves, 0);
//	while (!moves.empty())
//	{
//		auto curr_number_of_moves = moves.size();
//		for (Move move: moves)
//		{
//			int budget = static_cast<int> (n_playouts / (curr_number_of_moves * log2(number_of_moves)));
//			for (size_t i = 0; i < budget; i++)
//			{
//				CheckerBoard copy_state = state;
//				copy_state.play(move);
//				double res = UCT(copy_state, table);
//
//			}
//		}
//	}
//
//	return Move();
//}






template <class T, std::size_t N>
ostream& operator<<(ostream& o, const array<T, N>& arr) // Simple method to print arrays with "cout << array"
{
	copy(begin(arr), end(arr), ostream_iterator<T>(o, " "));
	return o;
}


int main()
{

	default_random_engine generator(rd());
	for (size_t i = 0; i < 8; i++) {
		hashTable.push_back(vector<vector<size_t>>{});
		for (size_t j = 0; j < 8; j++) {
			hashTable[i].push_back(vector<size_t>{});
			for (size_t k = 0; k < 4; k++)
				hashTable[i][j].push_back(int_distribution(generator));
		}
	}
	hashTurn = int_distribution(generator);


	using std::chrono::high_resolution_clock;
	using std::chrono::duration_cast;
	using std::chrono::duration;
	using std::chrono::milliseconds;
	using std::chrono::microseconds;
	CheckerBoard state{};
	
	auto t1 = high_resolution_clock::now();
	for (size_t i = 0; i < 500; i++)
	{
		CheckerBoard copy_state{};
		auto end_goal = copy_state.playout(generator);
	}
	auto t2 = high_resolution_clock::now();

	auto ms_int = duration_cast<milliseconds>(t2 - t1);
	std::cout << ms_int.count() << "ms\n";
	{
		CheckerBoard copy_state{};
		t1 = high_resolution_clock::now();
		auto selected_move = flat(state, 500);
		t2 = high_resolution_clock::now();
		ms_int = duration_cast<milliseconds>(t2 - t1);
		std::cout << ms_int.count() << "ms\n";
	}

	{
		CheckerBoard copy_state{};
		t1 = high_resolution_clock::now();
		auto selected_move = bestMoveUCT(state, 500);
		t2 = high_resolution_clock::now();
		ms_int = duration_cast<milliseconds>(t2 - t1);
		std::cout << ms_int.count() << "ms\n";
	}

	{
		CheckerBoard copy_state{};
		t1 = high_resolution_clock::now();
		auto selected_move = bestMoveGRAVE(state, 500);
		t2 = high_resolution_clock::now();
		ms_int = duration_cast<milliseconds>(t2 - t1);
		std::cout << ms_int.count() << "ms\n";
	}


	//auto full_t1 = high_resolution_clock::now();

	//ofstream outfile;
	////double C1 = 1.42, C2 = 0.4;
	//const array<double, 8> C = { 0.0,0.2, 0.4,0.6,0.8,1.0,1.42,10.0 };
	//for (auto C1: C)
	//{
	//	for (auto C2 : C)
	//	{
	//		if (C1 >= C2) continue;



	//		int white_win = 0;

	//		array<int, MAX_THREADS> white_wins{};
	//		array<int, MAX_THREADS> black_wins{};
	//		array<int, MAX_THREADS> stalemates{};

	//		outfile.open(format("test_{}_{}.txt",C1,C2));
	//		if (!outfile.is_open()) {
	//			perror("Error open");
	//			exit(EXIT_FAILURE);
	//		}


	//		#pragma omp parallel for num_threads(MAX_THREADS) schedule(dynamic)
	//		for (int i = 0; i < 100; i++)
	//		{

	//			int tid = omp_get_thread_num();

	//			CheckerBoard copy_state{};
	//			/*cout << copy_state.print_board();
	//			{
	//				auto moves = copy_state.legal_moves();
	//				for (auto& move : moves)
	//				{
	//					auto to_print = format("({},{})->({},{})", move.x1, move.y1, move.x2, move.y2);
	//					cout << to_print;
	//					for (auto eat : move.piecesToRemove)
	//						cout << eat.first << " " << eat.second;

	//				}
	//			}*/

	//			while (!copy_state.terminal()) {
	//				if (copy_state.white_turn) {
	//					Move move = bestMoveUCT(copy_state, 512, C1);
	//					copy_state.play(move);
	//				}
	//				else {
	//					Move move = bestMoveUCT(copy_state, 512, C2);
	//					copy_state.play(move);
	//				}
	//				/*cout << copy_state.print_board();
	//				auto moves = copy_state.legal_moves();
	//				for (auto& move : moves)
	//				{
	//					auto to_print = format("({},{})->({},{})", move.x1, move.y1, move.x2, move.y2);
	//					cout << to_print;
	//					for (auto eat : move.piecesToRemove)
	//						cout << eat.first << " " << eat.second;

	//				}
	//				cout << '\n';*/
	//			}
	//			//cout << copy_state.print_board();
	//			//auto moves = copy_state.legal_moves();
	//			/*for (auto& move : moves)
	//			{
	//				auto to_print = format("({},{})->({},{})", move.x1, move.y1, move.x2, move.y2);
	//				cout << to_print;
	//				for (auto eat : move.piecesToRemove)
	//					cout << eat.first << " " << eat.second;

	//			}
	//			cout << '\n';*/
	//			double sp = copy_state.score();
	//			if (sp == 1.0) {
	//				white_wins[tid] += 1;
	//			}
	//			else if (sp == 0.0) {
	//				black_wins[tid] += 1;
	//			}
	//			else {
	//				stalemates[tid] += 1;
	//			}
	//		}
	//		int w = 0;
	//		int b = 0;
	//		int stale = 0;
	//		for (size_t i = 0; i < MAX_THREADS; i++)
	//		{
	//			w += white_wins[i];
	//			b += black_wins[i];
	//			stale += stalemates[i];
	//			//std::cout << ms_ints[i].count() << "ms\n";
	//		}
	//		outfile << C1 << " " << C2 << " White: " << w << " Black: " << b << " Stalemate: " << stale << '\n';
	//		auto full_t2 = high_resolution_clock::now();
	//		std::cout << duration_cast<milliseconds>(full_t2 - full_t1) << "ms\n";

	//		outfile.close();
	//	}

	//}

}
