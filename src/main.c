#include <pebble.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum {HEART, DIAMOND, SPADE, CLUB} suit;
	
Window *main_window;
static TextLayer *start_text_layer;
static TextLayer *difficulty_text_layer;
static TextLayer *instruction_text_layer;
static TextLayer *bet_text_layer;
static TextLayer *game_text_layer;
static TextLayer *results_text_layer;
static BitmapLayer *card_outline_layer;
static BitmapLayer *card_suit_layer;
static TextLayer *card_value_layer;

// Card image
//static GBitmap *card_outline_blank;
//static GBitmap *card_suit_blank;
static GBitmap *card_outline;
static GBitmap *card_suit_heart;
static GBitmap *card_suit_diamond;
static GBitmap *card_suit_spade;
static GBitmap *card_suit_club;

// Action bar icons
ActionBarLayer *action_bar;
static GBitmap *ab_h;
static GBitmap *ab_s;
static GBitmap *ab_d;
static GBitmap *ab_check;
static GBitmap *ab_plus;
static GBitmap *ab_minus;
static GBitmap *ab_blank;

//Global Variables
bool gameFinished = false;
int game_stage = 0;
int startingBank[4];
int currentBank;
int currentBet;
static char bet_str[64]; // used to display the current bet
int difficulty = 1; // 1 -> easy
					// 2 -> medium
					// 3 -> hard


struct card {
	int face;
	int suit;
	int value;
	bool drawn;
};

struct player {
	int score;
	int numAces;
	bool busted;
};

// Players
struct player Dealer;
struct player User;

struct card cardDeck[52+1];

int getValueFromFace(int face) {
	if (face <= 10)
		return face;
	else if (face <= 13)
		return 10;
	else
		return 11;
}

void setupBank() {
	startingBank[0] = 0;
	startingBank[1] = 1000;
	startingBank[2] = 500;
	startingBank[3] = 200;
	currentBet = 10;
	currentBank = 100;
}

void setup_icons() {
	ab_h = gbitmap_create_with_resource(RESOURCE_ID_A_B_H);
	ab_s = gbitmap_create_with_resource(RESOURCE_ID_A_B_S);
	ab_d = gbitmap_create_with_resource(RESOURCE_ID_A_B_D);
	ab_check = gbitmap_create_with_resource(RESOURCE_ID_A_B_CHECK);
	ab_plus = gbitmap_create_with_resource(RESOURCE_ID_A_B_PLUS);
	ab_minus = gbitmap_create_with_resource(RESOURCE_ID_A_B_MINUS);
	ab_blank = gbitmap_create_with_resource(RESOURCE_ID_A_B_BLANK);
}

void change_icon() {
	if (game_stage == 4 || game_stage == 2) {
		action_bar_layer_set_icon(action_bar, BUTTON_ID_UP, ab_h);
		action_bar_layer_set_icon(action_bar, BUTTON_ID_SELECT, ab_s);
		action_bar_layer_set_icon(action_bar, BUTTON_ID_DOWN, ab_d);
	} else if (game_stage == 1 || game_stage == 3) {
		action_bar_layer_set_icon(action_bar, BUTTON_ID_UP, ab_plus);
		action_bar_layer_set_icon(action_bar, BUTTON_ID_SELECT, ab_check);
		action_bar_layer_set_icon(action_bar, BUTTON_ID_DOWN, ab_minus);
	} else {
		action_bar_layer_set_icon(action_bar, BUTTON_ID_UP, ab_blank);
		action_bar_layer_set_icon(action_bar, BUTTON_ID_SELECT, ab_check);
		action_bar_layer_set_icon(action_bar, BUTTON_ID_DOWN, ab_blank);
	}
}

void displayCard(suit mySuit, int value) {
	
}

void clearCard() {
	
}

void clearLayers() {
	text_layer_set_text(start_text_layer, "");
	text_layer_set_text(difficulty_text_layer, "");
	text_layer_set_text(instruction_text_layer, "");
	text_layer_set_text(bet_text_layer, "");
	text_layer_set_text(game_text_layer, "");
	text_layer_set_text(results_text_layer, "");
	layer_set_hidden((Layer *)card_outline_layer, true);
	layer_set_hidden((Layer *)card_suit_layer, true);
	text_layer_set_text(card_value_layer, "");
}

void setupStage() {
	clearLayers();
	if (game_stage == 0) {
		text_layer_set_text(start_text_layer, "Welcome\nto\nBlackjack!");
	} else if (game_stage == 1) {
		if (difficulty == 1) {
			text_layer_set_text(difficulty_text_layer, "Difficulty:\nEasy");
		} else if (difficulty == 2) {
			text_layer_set_text(difficulty_text_layer, "Difficulty:\nMedium");
		} else if (difficulty == 3) {
			text_layer_set_text(difficulty_text_layer, "Difficulty:\nHard");
		}
	} else if (game_stage == 2) {
		text_layer_set_text(instruction_text_layer, "How to play\n\nButtons:\nH->Hit\nS->Stand\nD->Double Down\n\nClick S to continue");
	} else if (game_stage == 3) {
		snprintf(bet_str, 60, "Bank: $%d \n\nEnter bet:\n $%d", currentBank, currentBet);
		APP_LOG(APP_LOG_LEVEL_DEBUG, "setupStage str:");
		APP_LOG(APP_LOG_LEVEL_DEBUG, bet_str);
		text_layer_set_text(bet_text_layer, bet_str);
	} else if (game_stage == 4) {
		text_layer_set_text(game_text_layer, "Game will be\nplayed here!\nClick S\nto continue");
	} else if (game_stage == 9) {
		text_layer_set_text(results_text_layer, "Results\n\ngo\n\nhere!");
	} else {
		//nothing
	}
}

void setupDeck() {
	for (int cur_suit=1; cur_suit<=4; cur_suit++) {
		for (int cur_face=2; cur_face<=14; cur_face++) {
			cardDeck[((cur_suit-1)*13)+cur_face].suit = cur_suit;
			cardDeck[((cur_suit-1)*13)+cur_face].face = cur_face;
			cardDeck[((cur_suit-1)*13)+cur_face].value = getValueFromFace(cur_face);
			cardDeck[((cur_suit-1)*13)+cur_face].drawn = false;
		}
	}
}

int hit(struct player *myPlayer) {
	int player_score = myPlayer->score;
	int num_aces = myPlayer->numAces;
	int myCard = 0;
	bool validCard = false;
	while (!validCard) {
		srand(time(0));
		int random_number = rand();
		int random_card = (random_number%50)+2;
		if (!cardDeck[random_card].drawn) {
			myCard = random_card;
			cardDeck[myCard].drawn = true;
			validCard = true;
		}
	}
	player_score += cardDeck[myCard].value;
	if (cardDeck[myCard].face == 14)
		num_aces++;
	
	
	// TODO: draw a randow card from deck
	if (player_score > 21 && num_aces == 0) {
		myPlayer->busted = true;
	} else if (player_score > 21 && num_aces > 0) {
		num_aces--;
		player_score = player_score - 10;
	}
	
	// changing values to new values
	myPlayer->score = player_score;
	myPlayer->numAces = num_aces;
	return myCard;
}

bool shouldHit(struct player *myPlayer) {
	bool ret = false;
	if (myPlayer->score < 17) {
		if (myPlayer->score < 11) {
			ret = true;
		}
		if (myPlayer->score < 15 && difficulty == 2) {
		//ret = true;
		}
	}
	return ret;
}

void displayCard (int card_number) {
	if (cardDeck[card_number].suit == 1)
		bitmap_layer_set_bitmap(card_suit_layer, card_suit_heart);	
	else if (cardDeck[card_number].suit == 2)
		bitmap_layer_set_bitmap(card_suit_layer, card_suit_diamond);	
	else if (cardDeck[card_number].suit == 3)
		bitmap_layer_set_bitmap(card_suit_layer, card_suit_spade);	
	else if (cardDeck[card_number].suit == 4)
		bitmap_layer_set_bitmap(card_suit_layer, card_suit_club);
	layer_set_hidden((Layer *)card_outline_layer, false);
	layer_set_hidden((Layer *)card_suit_layer, false);
}

void btn_hit() {
	game_stage = 5;
	int drawn_card = hit(&User);
	displayCard(drawn_card);
	change_icon();
	setupStage();
}

void btn_doubleDown() {
	game_stage = 6;
	int drawn_card = hit(&User);
	displayCard(drawn_card);
	change_icon();
	setupStage();
}

void startGame() {
	Dealer.score = 0;
	Dealer.numAces = 0;
	User.score = 0;
	User.numAces = 0;
	if (shouldHit(&Dealer))
		hit(&Dealer);
}

static void window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);

  start_text_layer = text_layer_create(GRect(0, 25, 124, 168));
  text_layer_set_text_color(start_text_layer, GColorBlack);
  text_layer_set_background_color(start_text_layer, GColorClear);
  text_layer_set_font(start_text_layer, fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD));
  text_layer_set_text_alignment(start_text_layer, GTextAlignmentCenter);
  layer_add_child(window_layer, text_layer_get_layer(start_text_layer));
	
  difficulty_text_layer = text_layer_create(GRect(0, 25, 124, 168));
  text_layer_set_text_color(difficulty_text_layer, GColorBlack);
  text_layer_set_background_color(difficulty_text_layer, GColorClear);
  text_layer_set_font(difficulty_text_layer, fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD));
  text_layer_set_text_alignment(difficulty_text_layer, GTextAlignmentCenter);
  layer_add_child(window_layer, text_layer_get_layer(difficulty_text_layer));
	
  instruction_text_layer = text_layer_create(GRect(0, 5, 124, 168));
  text_layer_set_text_color(instruction_text_layer, GColorBlack);
  text_layer_set_background_color(instruction_text_layer, GColorClear);
  text_layer_set_font(instruction_text_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18));
  text_layer_set_text_alignment(instruction_text_layer, GTextAlignmentCenter);
  layer_add_child(window_layer, text_layer_get_layer(instruction_text_layer));
	
  bet_text_layer = text_layer_create(GRect(0, 25, 124, 168));
  text_layer_set_text_color(bet_text_layer, GColorBlack);
  text_layer_set_background_color(bet_text_layer, GColorClear);
  text_layer_set_font(bet_text_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18));
  text_layer_set_text_alignment(bet_text_layer, GTextAlignmentCenter);
  layer_add_child(window_layer, text_layer_get_layer(bet_text_layer));
	
  game_text_layer = text_layer_create(GRect(0, 0, 144, 168));
  text_layer_set_text_color(game_text_layer, GColorBlack);
  text_layer_set_background_color(game_text_layer, GColorClear);
  text_layer_set_font(game_text_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18));
  text_layer_set_text_alignment(game_text_layer, GTextAlignmentCenter);
  layer_add_child(window_layer, text_layer_get_layer(game_text_layer));
	
  results_text_layer = text_layer_create(GRect(0, 0, 144, 168));
  text_layer_set_text_color(results_text_layer, GColorBlack);
  text_layer_set_background_color(results_text_layer, GColorClear);
  text_layer_set_font(results_text_layer, fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD));
  text_layer_set_text_alignment(results_text_layer, GTextAlignmentCenter);
  layer_add_child(window_layer, text_layer_get_layer(results_text_layer));

  card_outline = gbitmap_create_with_resource(RESOURCE_ID_CARD_OUTLINE);
  card_suit_heart = gbitmap_create_with_resource(RESOURCE_ID_SUIT_HEART);
  card_suit_diamond = gbitmap_create_with_resource(RESOURCE_ID_SUIT_DIAMOND);
  card_suit_spade = gbitmap_create_with_resource(RESOURCE_ID_SUIT_SPADE);
  card_suit_club = gbitmap_create_with_resource(RESOURCE_ID_SUIT_CLUB);

  card_outline_layer = bitmap_layer_create(GRect(20, 20, 104, 132));
  layer_set_hidden((Layer *)card_outline_layer, false);
  bitmap_layer_set_bitmap(card_outline_layer, card_outline);
  bitmap_layer_set_alignment(card_outline_layer, GAlignCenter);
  layer_add_child(window_layer, bitmap_layer_get_layer(card_outline_layer));
	
  card_suit_layer = bitmap_layer_create(GRect(43, 30, 81, 71));
  layer_set_hidden((Layer *)card_suit_layer, true);
  bitmap_layer_set_bitmap(card_suit_layer, card_suit_heart);
  bitmap_layer_set_alignment(card_suit_layer, GAlignCenter);
  layer_add_child(window_layer, bitmap_layer_get_layer(card_suit_layer));
	
  card_value_layer = text_layer_create(GRect(43, 80, 81, 115));
  text_layer_set_text_color(card_value_layer, GColorBlack);
  text_layer_set_background_color(card_value_layer, GColorClear);
  text_layer_set_font(card_value_layer, fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD));
  text_layer_set_text_alignment(card_value_layer, GTextAlignmentCenter);
  layer_add_child(window_layer, text_layer_get_layer(card_value_layer));

}

static void window_unload(Window *window) {
	text_layer_destroy(start_text_layer);
	text_layer_destroy(difficulty_text_layer);
	text_layer_destroy(instruction_text_layer);
	text_layer_destroy(bet_text_layer);
	text_layer_destroy(game_text_layer);
	text_layer_destroy(results_text_layer);
	text_layer_destroy(card_value_layer);
	bitmap_layer_destroy(card_outline_layer);
	bitmap_layer_destroy(card_suit_layer);
}


void up_single_click_handler(ClickRecognizerRef recognizer, Window *window) {
  	if (game_stage == 1) {
		if (difficulty == 1) {
			text_layer_set_text(difficulty_text_layer, "Difficulty:\nMedium");
			difficulty = 2;
		} else if (difficulty == 2) {
			text_layer_set_text(difficulty_text_layer, "Difficulty:\nHard");
			difficulty = 3;
		} else if (difficulty == 3) {
			text_layer_set_text(difficulty_text_layer, "Difficulty:\nEasy");
			difficulty = 1;
		}
	} else if (game_stage == 3) {
		if (currentBet < currentBank) {
			currentBet++;
			setupStage();
		}
	} else if (game_stage == 4) {
		btn_hit();
	}
}

void down_single_click_handler(ClickRecognizerRef recognizer, Window *window) {
  	if (game_stage == 1) {
		if (difficulty == 1) {
			text_layer_set_text(difficulty_text_layer, "Difficulty:\nHard");
			difficulty = 3;
		} else if (difficulty == 2) {
			text_layer_set_text(difficulty_text_layer, "Difficulty:\nEasy");
			difficulty = 1;
		} else if (difficulty == 3) {
			text_layer_set_text(difficulty_text_layer, "Difficulty:\nMedium");
			difficulty = 2;
		}
	} else if (game_stage == 3) {
		if (currentBet > 1) {
			currentBet--;
			setupStage();
		}
	} else if (game_stage == 4) {
		btn_doubleDown();
	}
}

void select_single_click_handler(ClickRecognizerRef recognizer, Window *window) {
	if (game_stage == 0 || game_stage == 1 || game_stage == 2) {
		if (game_stage == 1) {
			currentBank = startingBank[difficulty];
		}
		game_stage++;
		change_icon();
		setupStage();
	} else if (game_stage == 3) {
		game_stage++;
		change_icon();
		setupStage();
		startGame();
	} else if (game_stage == 4) {
		game_stage = 9;
		change_icon();
		setupStage();
	} else if (game_stage == 5 || game_stage == 6) {
		game_stage = 4;
		change_icon();
		setupStage();
	}
}

void config_provider(Window *window)
{
        window_single_click_subscribe(BUTTON_ID_UP, (ClickHandler) up_single_click_handler);
        window_single_click_subscribe(BUTTON_ID_SELECT, (ClickHandler) select_single_click_handler);
        window_single_click_subscribe(BUTTON_ID_DOWN, (ClickHandler) down_single_click_handler);
}

void init(void) {
	main_window = window_create();
	window_set_background_color(main_window, GColorWhite);
	window_set_fullscreen(main_window, false);
	window_set_window_handlers(main_window, (WindowHandlers) {
		.load = window_load,
		.unload = window_unload
	});
	action_bar = action_bar_layer_create();
	action_bar_layer_add_to_window(action_bar, main_window);
	const bool animated = true;
	window_stack_push(main_window, animated);
	setup_icons();
	setupDeck();
	change_icon();
	setupBank();
	setupStage();
}

void deinit(void) {
	gbitmap_destroy(ab_h);
	gbitmap_destroy(ab_s);
	gbitmap_destroy(ab_d);
	gbitmap_destroy(ab_check);
	gbitmap_destroy(ab_plus);
	gbitmap_destroy(ab_minus);
	gbitmap_destroy(ab_blank);
	gbitmap_destroy(card_suit_heart);
	gbitmap_destroy(card_suit_diamond);
	gbitmap_destroy(card_suit_spade);
	gbitmap_destroy(card_suit_club);
	gbitmap_destroy(card_outline);
	window_destroy(main_window);
}

int main(void) {
	init();
	window_set_click_config_provider(main_window, (ClickConfigProvider) config_provider);
	app_event_loop();
	deinit();
}
