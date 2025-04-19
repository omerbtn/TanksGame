#include "board.h"

#include "cell.h"
#include "shell.h"

Board::Board(int width, int height) : width(width), height(height) {}

Cell* Board::getCell(int x, int y) {
	return board[x][y];
}

void Board::addShell(Shell* shell) {
	//TODO
}