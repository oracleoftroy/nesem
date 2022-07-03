#pragma once

#include <string_view>

#include <cm/math.hpp>
#include <ui/canvas.hpp>

void draw_char(ui::Canvas &canvas, cm::Color color, char ch, cm::Point2i pos);
void draw_string(ui::Canvas &canvas, cm::Color color, std::string_view text, cm::Point2i pos);
void draw_string_centered(ui::Canvas &canvas, cm::Color color, std::string_view text, cm::Recti area);
