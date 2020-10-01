#include "decider.h"

#include "../chess/game.h"
#include <iostream>

// Decider class
std::pair<double, std::map<game::Move, double>> decider::Decider::prediction(game::Game *game) {
  game::Board *board = game->board();

  piece::PieceColor current_color = game->getCurrentColor();
  double color_multiplier = current_color.value();

  double currEval = color_multiplier * predictPosition(board);

  std::map<game::Move, double> actionMap;
  std::vector<game::Move> moves = game->possibleMoves(current_color);
  std::vector<game::Move> temp_vec;

  for (auto &move : moves)
    if (move.verify(board)) {
      board->doMove(new game::Move(move), game);

      if (current_color.isWhite())
        board->getPossibleMoves(nullptr, &temp_vec);
      else
        board->getPossibleMoves(&temp_vec, nullptr);

      if (temp_vec.empty())
        actionMap[move] = color_multiplier * predictPosition(board);
      else {
        actionMap[move] = color_multiplier * 20.0;
        temp_vec.clear();
      }
      board->undoMove(game);
    } else DEBUG_ASSERT

  return {currEval, actionMap};
}

// Randomizer class
double decider::Randomizer::predictPosition(game::Board *b) {
  double range = 4.0;
  return random(-range, range);
}

// Minimaxer class
double decider::Minimaxer::predictPosition(game::Board *b) {
  return b->score(
    [&](piece::Piece *piece, int r, int c) -> double {
      return piece->color().value() * piece->type().minimaxValue();//r, c, piece->color());
    }
  );
}