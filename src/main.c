#include <pebble.h>
#include <stdlib.h>
#include <time.h>

#define CELLS_COUNT 64
#define ROWS_COUNT 8
#define BOX_SIZE 17
#define START_CELL 0
#define MINES_COUNT 10
#define HEADING_OFFSET 8

const char* VISUAL_MINE  = "*";
const char* VISUAL_FLAG  = "+";
const char* VISUAL_CLEAR = "-";
const char* VISUAL_BLANK = "";

uint8_t MINE_MASK    = 0b00000001;
uint8_t FLAG_MASK    = 0b00000010;
uint8_t CLEARED_MASK = 0b00000100;

bool gameRunning = true;

uint8_t* cells;
char** threatCells;
uint8_t selectedCell = START_CELL;
uint8_t minesCount = MINES_COUNT;

Window *my_window;
TextLayer** boardDisplay;

void initBoard(uint8_t* loadedCells, char** loadedThreatCells) {
   //init empty board
   if (cells != NULL) free(cells);
   if (loadedCells != NULL) {
      cells = loadedCells;
   } else {
      cells = malloc(sizeof(uint8_t) * CELLS_COUNT);
   }
   
   for (int i = 0; i < CELLS_COUNT; i++)
   {
      cells[i] = 0;
   }
   
   if (threatCells != NULL) {
      for (uint8_t i = 0; i < CELLS_COUNT; i++) {
         if (threatCells[i] != NULL)
         {
            free(threatCells[i]);
            threatCells[i] = NULL;
         }
      }
   }
   if (threatCells == NULL)
   {
      if (loadedThreatCells != NULL) {
         threatCells = loadedThreatCells;
      } else {
         threatCells = malloc(sizeof(char*) * CELLS_COUNT);
      }
   }
   
   //reset mines from previous game
   minesCount = MINES_COUNT;
   
   //laying fresh mines
   uint8_t mineLocation;
   srand(time(NULL));
   while (minesCount > 0)
   {
      //pseudo random number between 0 and 63
      mineLocation = rand() % 64;
      if (cells[mineLocation] == 0)
      {
         cells[mineLocation] = MINE_MASK;
         minesCount--;
      }
   }
   
   if (boardDisplay != NULL)
   {
      for (uint8_t i = 0; i < CELLS_COUNT; i++)
      {
         text_layer_destroy(boardDisplay[i]);
         boardDisplay[i] = NULL;
      }
      free(boardDisplay);
      boardDisplay = NULL;
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
      
      //set blank for all the uncleared cells
      text_layer_set_text(boardDisplay[i], VISUAL_BLANK);
   }
   
   //GAAMMEEEE BEGIIIINNNNN!
   gameRunning = true;
}

void processFailureCondition() {
   if (((cells[selectedCell] & MINE_MASK) == MINE_MASK) && ((cells[selectedCell] & CLEARED_MASK) == CLEARED_MASK)) {
      //lost the game
      gameRunning = false;
      for (uint8_t i = 0; i < CELLS_COUNT; i++) {
         if ((cells[i] & MINE_MASK) == MINE_MASK) {
            text_layer_set_text(boardDisplay[i], VISUAL_MINE);
            if (i != selectedCell) text_layer_set_background_color(boardDisplay[i], GColorWhite);
            if (i != selectedCell) text_layer_set_text_color(boardDisplay[i], GColorBlack);
            text_layer_set_font(boardDisplay[i], fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD));
         } else {
            text_layer_set_text(boardDisplay[i], VISUAL_BLANK);
            text_layer_set_text_color(boardDisplay[i], GColorWhite);
            text_layer_set_background_color(boardDisplay[i], GColorWhite);
         }
      }
   } else {
      uint8_t unclearedCells = CELLS_COUNT;
      for (uint8_t i = 0; i < CELLS_COUNT; i++) {
         if ((cells[i] & CLEARED_MASK) == CLEARED_MASK) {
            unclearedCells--;
         }
      }
      if (MINES_COUNT >= unclearedCells) {
         //winner!
         gameRunning = false;
         for (uint8_t i = 0; i < CELLS_COUNT; i++) {
            if ((cells[i] & MINE_MASK) == MINE_MASK) {
               text_layer_set_text(boardDisplay[i], VISUAL_MINE);
               text_layer_set_background_color(boardDisplay[i], GColorWhite);
               text_layer_set_text_color(boardDisplay[i], GColorBlack);
            } else {
               text_layer_set_text(boardDisplay[i], VISUAL_BLANK);
               if (i != selectedCell) text_layer_set_background_color(boardDisplay[i], GColorWhite);
               text_layer_set_text_color(boardDisplay[i], GColorWhite);
            }
         }
      }
   }
}

void selectedRedrawBoardDisplay(uint8_t cell) {
   if ((cells[cell] & CLEARED_MASK) == CLEARED_MASK) {
      uint8_t col = cell % ROWS_COUNT;
      uint8_t row = cell / ROWS_COUNT;
      
      uint8_t neighbourCellsCount = 8;
      if ((col == (ROWS_COUNT-1)) || (col == 0)) {
         neighbourCellsCount -= 3;
      }
      if ((row == (ROWS_COUNT-1)) || (row == 0)) {
         if (neighbourCellsCount < 8) {
            neighbourCellsCount -= 2;
         } else {
            neighbourCellsCount -= 3;
         }
      }
      
      uint8_t *neighbourCells = malloc(sizeof(uint8_t)*neighbourCellsCount);
      uint8_t neighbourFillIndex = 0;
      //loop through neighbouring cells, adding the valid cells to the new array
      if ((col > 0) && (row > 0)) {
         neighbourCells[neighbourFillIndex] = cell-ROWS_COUNT-1;
         neighbourFillIndex++;
      }
      if ((col < ROWS_COUNT-1) && (row < ROWS_COUNT-1)) {
         neighbourCells[neighbourFillIndex] = cell+ROWS_COUNT+1;
         neighbourFillIndex++;
      }
      if ((col < ROWS_COUNT-1) && (row > 0)) {
         neighbourCells[neighbourFillIndex] = cell-ROWS_COUNT+1;
         neighbourFillIndex++;
      }
      if ((col > 0) && (row < ROWS_COUNT-1)) {
         neighbourCells[neighbourFillIndex] = cell+ROWS_COUNT-1;
         neighbourFillIndex++;
      }
      if (col > 0) {
         neighbourCells[neighbourFillIndex] = cell-1;
         neighbourFillIndex++;
      }
      if (col < ROWS_COUNT-1) {
         neighbourCells[neighbourFillIndex] = cell+1;
         neighbourFillIndex++;
      }
      if (row > 0) {
         neighbourCells[neighbourFillIndex] = cell-ROWS_COUNT;
         neighbourFillIndex++;
      }
      if (row < ROWS_COUNT-1) {
         neighbourCells[neighbourFillIndex] = cell+ROWS_COUNT;
      }
      
      uint8_t localTerrorists = 0;
      for (uint8_t i = 0; i < neighbourCellsCount; i++) {
         //counting the neighbouring bombs
         if ((cells[neighbourCells[i]] & MINE_MASK) == MINE_MASK) {
            localTerrorists++;
         }
      }
      
      if (localTerrorists > 0) {
         //set the bomb count to the threatCells array
         threatCells[cell] = malloc(sizeof(char)*2);
         snprintf(threatCells[cell], sizeof(char)*2, "%d", localTerrorists);
         text_layer_set_text(boardDisplay[cell], threatCells[cell]);
         text_layer_set_font(boardDisplay[cell], fonts_get_system_font(FONT_KEY_GOTHIC_14_BOLD));
      } else {
         text_layer_set_text(boardDisplay[cell], VISUAL_CLEAR);
         for (uint8_t i = 0; i < neighbourCellsCount; i++) {
            //only do this if the cell has not already been cleared
            if ((cells[neighbourCells[i]] & CLEARED_MASK) != CLEARED_MASK) {
               cells[neighbourCells[i]] |= CLEARED_MASK;
               selectedRedrawBoardDisplay(neighbourCells[i]);
            }
         }
      }
      
      free(neighbourCells);
      neighbourCells = NULL;
      
   } else {
      if ((cells[cell] & FLAG_MASK) == FLAG_MASK) {
         text_layer_set_text(boardDisplay[cell], VISUAL_FLAG);
         text_layer_set_font(boardDisplay[cell], fonts_get_system_font(FONT_KEY_GOTHIC_14_BOLD));
      } else {
         text_layer_set_text(boardDisplay[cell], VISUAL_BLANK);
         text_layer_set_font(boardDisplay[cell], fonts_get_system_font(FONT_KEY_GOTHIC_14));
      }
   }
}

void toggleCellFlagClick() {
   if (((cells[selectedCell] & CLEARED_MASK) != CLEARED_MASK)) {
      cells[selectedCell] = cells[selectedCell] ^ FLAG_MASK;
      selectedRedrawBoardDisplay(selectedCell);
   }
}
void moveSelectedCellClick(uint8_t oldSelectedCell) {
   if (oldSelectedCell != selectedCell) {
      text_layer_set_text_color(boardDisplay[oldSelectedCell], GColorBlack);
      text_layer_set_background_color(boardDisplay[oldSelectedCell], GColorWhite);
      
      text_layer_set_text_color(boardDisplay[selectedCell], GColorWhite);
      text_layer_set_background_color(boardDisplay[selectedCell], GColorBlack);
   }
}

void handler_single_click_up(ClickRecognizerRef recognizer, void *context) {
   if (gameRunning) {
      uint8_t oldSelectedCell = selectedCell;
      if ((selectedCell % ROWS_COUNT) == (ROWS_COUNT-1)) {
         selectedCell -= (ROWS_COUNT-1);
      } else {
         selectedCell++;
      }
      moveSelectedCellClick(oldSelectedCell);   
   }
}
void handler_long_click_up(ClickRecognizerRef recognizer, void *context) {
   gameRunning = false;
   initBoard(NULL, NULL);
}
void handler_long_click_release_up(ClickRecognizerRef recognizer, void *context) {
}
void handler_single_click_down(ClickRecognizerRef recognizer, void *context) {
   if (gameRunning) {
      uint8_t oldSelectedCell = selectedCell;
      if ((selectedCell / ROWS_COUNT) == (ROWS_COUNT-1)) {
         selectedCell -= (ROWS_COUNT*(ROWS_COUNT-1));
      } else {
         selectedCell += ROWS_COUNT;
      }
      moveSelectedCellClick(oldSelectedCell);
   }
}
void handler_single_click_select(ClickRecognizerRef recognizer, void *context) {
   if (gameRunning) {
      if ((((cells[selectedCell] & FLAG_MASK) != FLAG_MASK)) && (((cells[selectedCell] & CLEARED_MASK) != CLEARED_MASK))) {
         cells[selectedCell] |= CLEARED_MASK;
         selectedRedrawBoardDisplay(selectedCell);
         processFailureCondition();
      }
   }
}
void handler_long_click_select(ClickRecognizerRef recognizer, void *context) {
   if (gameRunning) {
      toggleCellFlagClick();
   }
}
void handler_long_click_release_select(ClickRecognizerRef recognizer, void *context) {
}
void config_provider(Window *window) {
   window_single_click_subscribe(BUTTON_ID_UP, handler_single_click_up);
   window_long_click_subscribe(BUTTON_ID_UP, 500, handler_long_click_up, handler_long_click_release_up);
   window_single_click_subscribe(BUTTON_ID_DOWN, handler_single_click_down);
   window_single_click_subscribe(BUTTON_ID_SELECT, handler_single_click_select);
   window_long_click_subscribe(BUTTON_ID_SELECT, 500, handler_long_click_select, handler_long_click_release_select);
}

uint8_t* getPersistedCells() {
   return NULL;
}
char** getPersistedThreatCells() {
   return NULL;
}

void handle_init(void) {
   my_window = window_create();
   
   uint8_t* pCells = getPersistedCells();
   char** pThreadCells = getPersistedThreatCells();
   initBoard(pCells, pThreadCells);
   
   window_set_click_config_provider(my_window, (ClickConfigProvider) config_provider);
   window_stack_push(my_window, true);
}
void handle_deinit(void) {
   for (uint8_t i = 0; i < CELLS_COUNT; i++)
   {
      if (boardDisplay[i] != NULL) text_layer_destroy(boardDisplay[i]);
   }
   
   if (boardDisplay != NULL) free(boardDisplay);
   
   if (cells != NULL) free(cells);
   
   if (threatCells != NULL) {
      for (uint8_t i = 0; i < CELLS_COUNT; i++) {
         if (threatCells[i] != NULL) free(threatCells[i]);
      }
      free(threatCells);
   }
   
   window_destroy(my_window);
}
int main(void) {
   handle_init();
   app_event_loop();
   handle_deinit();
}
