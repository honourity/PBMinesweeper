#include <pebble.h>
#include <stdlib.h>
#include <time.h>

#define CELLS_COUNT 64
#define ROWS_COUNT 8
#define BOX_SIZE 17
#define START_CELL 0
#define MINES_COUNT 10
#define HEADING_OFFSET 8

uint8_t MINE_MASK = 0b00000001;
uint8_t CLEARED_MASK = 0b00000100;
//uint8_t SPARE_MASK = 0b00001000;
uint8_t FLAG_MASK = 0b00000010;
uint8_t HINT_MASK = 0b11110000;

bool gameRunning = true;

uint8_t* cells;
uint8_t selectedCell = START_CELL;
uint8_t minesCount = MINES_COUNT;

Window *my_window;
TextLayer** boardDisplay;

void initBoard()
{
   //init empty board
   if (cells != NULL) free(cells);
   cells = malloc(sizeof(uint8_t) * CELLS_COUNT);
   for (int i = 0; i < CELLS_COUNT; i++)
   {
      cells[i] = 0b00000000;
   }
   
   //laying mines
   uint8_t mineLocation;
   srand(time(NULL));
   while (minesCount > 0)
   {
      //pseudo random number between 0 and 63
      mineLocation = rand() % 64;
      if (cells[mineLocation] == 0)
      {
         cells[mineLocation] = 0b00000001;
         minesCount--;
      }
   }
   
   if (boardDisplay != NULL)
   {
      for (uint8_t i = 0; i < CELLS_COUNT; i++)
      {
         text_layer_destroy(boardDisplay[i]);
      }
      free(boardDisplay);
   }
   
   window_set_background_color(my_window, GColorBlack);
   Layer *window_layer = window_get_root_layer(my_window);
   
   boardDisplay = malloc(sizeof(struct TextLayer *) * CELLS_COUNT);
   
   //set default values
   for (uint8_t i = 0; i < CELLS_COUNT; i++)
   {
      uint8_t col = i % ROWS_COUNT;
      uint8_t row = i / ROWS_COUNT;
      short hStartPos = col * (BOX_SIZE + 1);
      short vStartPos = row * (BOX_SIZE + 1);
      
      boardDisplay[i] = text_layer_create(GRect(hStartPos+1, vStartPos+1+HEADING_OFFSET, BOX_SIZE-1, BOX_SIZE-1));
      text_layer_set_font(boardDisplay[i], fonts_get_system_font(FONT_KEY_GOTHIC_14));

      layer_add_child(window_layer, text_layer_get_layer(boardDisplay[i]));
   }
   text_layer_set_background_color(boardDisplay[selectedCell], GColorBlack);
   
   //draw the board display
   for (uint8_t i = 0; i < CELLS_COUNT; i++)
   {
      text_layer_set_text_alignment(boardDisplay[i], GTextAlignmentCenter);
      
      if (i == selectedCell)
      {
         text_layer_set_text_color(boardDisplay[i], GColorWhite);
         text_layer_set_background_color(boardDisplay[i], GColorBlack);
      }
      else
      {
         text_layer_set_text_color(boardDisplay[i], GColorBlack);
         text_layer_set_background_color(boardDisplay[i], GColorWhite);
      }
      
      if ((cells[i] & CLEARED_MASK) == CLEARED_MASK)
      {
         text_layer_set_text(boardDisplay[i], "~");
      }
      else
      {
         if ((cells[i] & FLAG_MASK) == FLAG_MASK)
         {
            text_layer_set_text(boardDisplay[i], "!");
         }
      }
   }
}

void processFailureCondition()
{
   if (((cells[selectedCell] & MINE_MASK) == MINE_MASK) && ((cells[selectedCell] & CLEARED_MASK) == CLEARED_MASK))
   {
      //lost the game
      gameRunning = false;
      
      for (uint8_t i = 0; i < CELLS_COUNT; i++)
      {
         if ((cells[i] & MINE_MASK) == MINE_MASK)
         {
            text_layer_set_text(boardDisplay[i], "*");
            text_layer_set_background_color(boardDisplay[i], GColorBlack);
            text_layer_set_text_color(boardDisplay[i], GColorWhite);
         }
         else
         {
            text_layer_set_text(boardDisplay[i], "");
            text_layer_set_background_color(boardDisplay[i], GColorBlack);
         }
      }
   }
   else
   {
      uint8_t unclearedCells = CELLS_COUNT;
      for (uint8_t i = 0; i < CELLS_COUNT; i++)
      {
         if ((cells[i] & CLEARED_MASK) == CLEARED_MASK)
         {
            unclearedCells--;
         }
      }
      if (MINES_COUNT >= unclearedCells)
      {
         //winner!
         gameRunning = false;
         
         for (uint8_t i = 0; i < CELLS_COUNT; i++)
         {
            if ((cells[i] & MINE_MASK) == MINE_MASK)
            {
               text_layer_set_text(boardDisplay[i], "*");
               text_layer_set_background_color(boardDisplay[i], GColorWhite);
               text_layer_set_text_color(boardDisplay[i], GColorBlack);
            }
            else
            {
               uint8_t cellThreatCount = cells[i] >> 4;
               if (cellThreatCount > 0)
               {
                  char charThing[3];
                  snprintf(charThing, sizeof(3), "%d", cellThreatCount);
                  text_layer_set_text(boardDisplay[i], charThing);
               }
               else
               {
                  text_layer_set_text(boardDisplay[i], "");
               }
               if (i != selectedCell) text_layer_set_background_color(boardDisplay[i], GColorWhite);
               text_layer_set_text_color(boardDisplay[i], GColorWhite);
            }
         }
      }
   }
}

//recurse boolean is to indicate that this function is calling itself
void selectedRedrawBoardDisplay(uint8_t cell)
{
   if ((cells[cell] & CLEARED_MASK) == CLEARED_MASK)
   {
      text_layer_set_text(boardDisplay[cell], "~");
      
      
      uint8_t col = cell % ROWS_COUNT;
      uint8_t row = cell / ROWS_COUNT;

      if ((col > 0) && (row > 0))
      {
         if ((((cells[cell-1-ROWS_COUNT] & MINE_MASK) == MINE_MASK) && !((cells[cell-1-ROWS_COUNT] & CLEARED_MASK) == CLEARED_MASK)))
         {
            //cells[cell-1-ROWS_COUNT] |= CLEARED_MASK;
            //selectedRedrawBoardDisplay(cell-1-ROWS_COUNT);
            uint8_t a = ((cells[cell] >> 4) + 1) << 4;
            uint8_t b = (cells[cell] << 4);
            uint8_t c = (b >> 4);
            cells[cell] = a | c;
         }
      }
      if ((col < ROWS_COUNT-1) && (row < ROWS_COUNT-1))
      {
         if ((((cells[cell+1+ROWS_COUNT] & MINE_MASK) == MINE_MASK) && !((cells[cell+1+ROWS_COUNT] & CLEARED_MASK) == CLEARED_MASK)))
         {
            //cells[cell+1+ROWS_COUNT] |= CLEARED_MASK;
            //selectedRedrawBoardDisplay(cell+1+ROWS_COUNT);
            uint8_t a = ((cells[cell] >> 4) + 1) << 4;
            uint8_t b = (cells[cell] << 4);
            uint8_t c = (b >> 4);
            cells[cell] = a | c;
         }
      }
      if ((col < ROWS_COUNT-1) && (row > 0))
      {
         if ((((cells[cell+1-ROWS_COUNT] & MINE_MASK) == MINE_MASK) && !((cells[cell+1-ROWS_COUNT] & CLEARED_MASK) == CLEARED_MASK)))
         {
            //cells[cell+1-ROWS_COUNT] |= CLEARED_MASK;
            //selectedRedrawBoardDisplay(cell+1-ROWS_COUNT);
            uint8_t a = ((cells[cell] >> 4) + 1) << 4;
            uint8_t b = (cells[cell] << 4);
            uint8_t c = (b >> 4);
            cells[cell] = a | c;
         }
      }
      if ((col > 0) && (row < ROWS_COUNT-1))
      {
         if ((((cells[cell-1+ROWS_COUNT] & MINE_MASK) == MINE_MASK) && !((cells[cell-1+ROWS_COUNT] & CLEARED_MASK) == CLEARED_MASK)))
         {
            //cells[cell-1+ROWS_COUNT] |= CLEARED_MASK;
            //selectedRedrawBoardDisplay(cell-1+ROWS_COUNT);
            uint8_t a = ((cells[cell] >> 4) + 1) << 4;
            uint8_t b = (cells[cell] << 4);
            uint8_t c = (b >> 4);
            cells[cell] = a | c;
         }
      }
      if (col > 0)
      {
         if ((((cells[cell-1] & MINE_MASK) == MINE_MASK) && !((cells[cell-1] & CLEARED_MASK) == CLEARED_MASK)))
         {
            //cells[cell-1] |= CLEARED_MASK;
            //selectedRedrawBoardDisplay(cell-1);
            uint8_t a = ((cells[cell] >> 4) + 1) << 4;
            uint8_t b = (cells[cell] << 4);
            uint8_t c = (b >> 4);
            cells[cell] = a | c;
         }
      }
      if (col < ROWS_COUNT-1)
      {
         if ((((cells[cell+1] & MINE_MASK) == MINE_MASK) && !((cells[cell+1] & CLEARED_MASK) == CLEARED_MASK)))
         {
            //cells[cell+1] |= CLEARED_MASK;
            //selectedRedrawBoardDisplay(cell+1);
            uint8_t a = ((cells[cell] >> 4) + 1) << 4;
            uint8_t b = (cells[cell] << 4);
            uint8_t c = (b >> 4);
            cells[cell] = a | c;
         }
      }
      if (row > 0)
      {
         if ((((cells[cell-ROWS_COUNT] & MINE_MASK) == MINE_MASK) && !((cells[cell-ROWS_COUNT] & CLEARED_MASK) == CLEARED_MASK)))
         {
            //cells[cell-ROWS_COUNT] |= CLEARED_MASK;
            //selectedRedrawBoardDisplay(cell-ROWS_COUNT);
            uint8_t a = ((cells[cell] >> 4) + 1) << 4;
            uint8_t b = (cells[cell] << 4);
            uint8_t c = (b >> 4);
            cells[cell] = a | c;
         }
      }
      if (row < ROWS_COUNT-1)
      {
         if ((((cells[cell+ROWS_COUNT] & MINE_MASK) == MINE_MASK) && !((cells[cell+ROWS_COUNT] & CLEARED_MASK) == CLEARED_MASK)))
         {
            //cells[cell+ROWS_COUNT] |= CLEARED_MASK;
            //selectedRedrawBoardDisplay(cell+ROWS_COUNT);
            uint8_t a = ((cells[cell] >> 4) + 1) << 4;
            uint8_t b = (cells[cell] << 4);
            uint8_t c = (b >> 4);
            cells[cell] = a | c;
         }
      }
      
      
      uint8_t cellThreatCount = cells[cell] >> 4;
      if (cellThreatCount > 0)
      {
         char charThing[3];
         snprintf(charThing, sizeof(3), "%d", cellThreatCount);
         text_layer_set_text(boardDisplay[cell], charThing);
      }
   }
   else
   {
      if ((cells[cell] & FLAG_MASK) == FLAG_MASK)
      {
         text_layer_set_text(boardDisplay[cell], "!");
      }
   }
}

void moveRedrawBoardDisplay(uint8_t oldSelectedCell)
{
   if (oldSelectedCell != selectedCell)
   {
      text_layer_set_text_color(boardDisplay[oldSelectedCell], GColorBlack);
      text_layer_set_background_color(boardDisplay[oldSelectedCell], GColorWhite);
      
      text_layer_set_text_color(boardDisplay[selectedCell], GColorWhite);
      text_layer_set_background_color(boardDisplay[selectedCell], GColorBlack);
   }
}

void handler_single_click_up(ClickRecognizerRef recognizer, void *context) {
   if (gameRunning)
   {
      uint8_t oldSelectedCell = selectedCell;
      if ((selectedCell % ROWS_COUNT) == (ROWS_COUNT-1))
      {
         selectedCell -= (ROWS_COUNT-1);
      }
      else
      {
         selectedCell++;
      }
      moveRedrawBoardDisplay(oldSelectedCell);   
   }
}
void handler_long_click_up(ClickRecognizerRef recognizer, void *context) {
}
void handler_long_click_release_up(ClickRecognizerRef recognizer, void *context) {
   gameRunning = true;
   initBoard();
}
void handler_single_click_down(ClickRecognizerRef recognizer, void *context) {
   if (gameRunning)
   {
      uint8_t oldSelectedCell = selectedCell;
      if ((selectedCell / ROWS_COUNT) == (ROWS_COUNT-1))
      {
         selectedCell -= (ROWS_COUNT*(ROWS_COUNT-1));
      }
      else
      {
         selectedCell += ROWS_COUNT;
      }
      moveRedrawBoardDisplay(oldSelectedCell);
   }
}
void handler_single_click_select(ClickRecognizerRef recognizer, void *context) {
   if (gameRunning)
   {
      cells[selectedCell] = cells[selectedCell] | CLEARED_MASK;
      selectedRedrawBoardDisplay(selectedCell);
      processFailureCondition();
   }
}
void handler_long_click_select(ClickRecognizerRef recognizer, void *context) {
}
void handler_long_click_release_select(ClickRecognizerRef recognizer, void *context) {
   if (gameRunning)
   {
      cells[selectedCell] ^= FLAG_MASK;
      selectedRedrawBoardDisplay(selectedCell);
   }
}
void config_provider(Window *window) {
   window_single_click_subscribe(BUTTON_ID_UP, handler_single_click_up);
   window_long_click_subscribe(BUTTON_ID_UP, 500, handler_long_click_up, handler_long_click_release_up);
   window_single_click_subscribe(BUTTON_ID_DOWN, handler_single_click_down);
   window_single_click_subscribe(BUTTON_ID_SELECT, handler_single_click_select);
   window_long_click_subscribe(BUTTON_ID_SELECT, 500, handler_long_click_select, handler_long_click_release_select);
}

void handle_init(void) {
   my_window = window_create();
   
   initBoard();
   
   window_set_click_config_provider(my_window, (ClickConfigProvider) config_provider);
   
   window_stack_push(my_window, true);
}

void handle_deinit(void) {
  for (uint8_t i = 0; i < CELLS_COUNT; i++)
   {
    text_layer_destroy(boardDisplay[i]);
  }
  
  window_destroy(my_window);
}

int main(void) {
  handle_init();
  app_event_loop();
  handle_deinit();
}
