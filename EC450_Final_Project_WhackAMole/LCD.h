/*
 * LCD.h
 *
 *  Created on: Apr 25, 2015
 *      Author: Larry
 */

#ifndef LCD_H_
#define LCD_H_

void TXString(char *string);
void ConfigureTimerUart(void);
void Transmit();
void print_current_score(unsigned int hits);
void opening_screen();
void reset_screen();
void game_over_screen(unsigned int current_score);
void high_score_screen(unsigned int highscore);
void new_high_score_screen();
void itoa(unsigned int val, char *str, unsigned int limit);

#endif /* LCD_H_ */
