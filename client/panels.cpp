#include <curses.h>
#include <panel.h>
#include <menu.h>
#include <mutex>
#include <cstdlib>
#include <unistd.h>
#include <thread>
#include <cstring>
#include <map>
#include <iostream>

#define ARRAY_SIZE(a) (sizeof(a)/sizeof(a[0]))

// Commands constants
const int UP_COMMAND = 0; 
const int DOWN_COMMAND = 1; 
const int CHOOSE_COMMAND = 2; 

// Menu options
char * general_choices[] = {
	"Connect",
	"Turn on all trains",
	"Turn off all trains",
	"Specific Train",
	"Specific Speed Control",
	"Exit"
};

char * train_choices[] = {
	"1",
	"2",
	"3",
	"4",
	"5",
	"6",
	"7",
	"Exit"
};

// Globals
PANEL * panels[4];
MENU * menus[3];
WINDOW * windows[4];
bool connected = false;
int command = 0;
std::map<PANEL *, int> panels_id;
std::string velocity_pot = "200";
std::mutex mtx;

int n_general_choices = ARRAY_SIZE(general_choices);
int n_train_choices = ARRAY_SIZE(train_choices);

// Arrays of functions for each panel
void (*gen_funcs[3])(int);					// Functions for general menu
void (*train_mturn_funcs[3])(int);			// Functions for turn train menu
void (*train_mspeed_funcs[3])(int);		// Functions for train speed menu
void (*train_speed_funcs[3])(int);			// Function for train speed window

/*------------- Functions for each panel ----------------*/
// General panel
//--------------------------------------------------------
//-- Commands --//
void up_general_menu(int);
void down_general_menu(int);
void choose_general_menu(int);

void up_general_menu(int p) {
	menu_driver(menus[p], REQ_UP_ITEM);	
}

void down_general_menu(int p) {
	menu_driver(menus[p], REQ_DOWN_ITEM);	
}

void choose_general_menu(int i) {
	ITEM * cur = (ITEM *) current_item(menus[i]);
	void (*p) (char *) = reinterpret_cast<void (*) (char *)>
		(item_userptr(cur));	
	if (p != nullptr)
		p((char *) item_name(cur));
}
//-- Items actions --//
void connection_management(char * name);
void turn_on_all(char * name);
void turn_off_all(char * name);
void turn(char * name);
void set_speed(char * name);
void exit_client(char * name);

void connection_management(char * name) {
	bool before = connected;

	if (!connected) {
		connected = true;
		general_choices[0] = "Disconnect";
	} else {
		connected = false;
		general_choices[0] = "Connect";
	}

	if (before != connected) {
		ITEM ** items = menu_items(menus[0]);
		unpost_menu(menus[0]);
		free_item(items[0]);
		items[0] = new_item(general_choices[0], general_choices[0]);
		set_item_userptr(items[0], reinterpret_cast<void *>(connection_management));
		set_menu_items(menus[0], items);
		post_menu(menus[0]);
		wrefresh(windows[0]);
	}
}

void turn_on_all(char * name) {

}

void turn_off_all(char * name) {

}

void turn(char * name) {
	top_panel(panels[1]);
}

void set_speed(char * name) {
	top_panel(panels[2]);
}

void exit_client(char * name) {
	command = -1;
	endwin();
}
//-----------------------------------
// Train turn panel
// ---------------------------------
//--- Itens actions ---//
void turn_train_menu(char * name);
void exit_train_panel(char * name);

void turn_train_menu(char * name) {

}

void exit_train_panel(char * name) {
	top_panel(panels[0]);
}

//-----------------------------------
// Train menu speed panel
// ---------------------------------
//--- Itens actions ---//
void speed_train_menu(char * name);

void speed_train_menu(char * name) {
	box(windows[3], 0, 0);
	show_panel(panels[3]);
	mvwprintw(windows[3], 0, 0, name);
	wrefresh(windows[3]);
}
//-----------------------------------
// Train speed panel
// ---------------------------------
//-- Commands --//
void speed_window_confirm(int);

void speed_window_confirm(int i) {
	hide_panel(panels[3]);
	wrefresh(windows[3]);
	top_panel(panels[2]);
}

/* Potentiometer thread
 * ---------------------- */
void potentiometer_update() {
	mtx.lock();
	while (panel_below((PANEL *)0) == panels[3]) {
		velocity_pot = "300"; // getpotentiometervalue();
		mvwprintw(windows[3], 1, 2, velocity_pot.c_str());
		wrefresh(windows[3]);
		usleep(5000);
	}
	mtx.unlock();
}

/* Main method.
 -----------------------------------------------*/
int main(void) {
	// Init ncurses
	initscr();
	start_color();
	cbreak();
	noecho();
	keypad(stdscr, TRUE);

	/*------------ Creating elements -------------*/
	// General menu
	ITEM * * general_items = (ITEM **) calloc(n_general_choices + 1, sizeof(ITEM *));
	for (int i = 0; i < n_general_choices; ++i)
		general_items[i] = new_item(general_choices[i], general_choices[i]);
	general_items[n_general_choices] = (ITEM *) NULL;
	set_item_userptr(general_items[0], reinterpret_cast<void *>(connection_management));
	set_item_userptr(general_items[1], reinterpret_cast<void *>(turn_on_all));
	set_item_userptr(general_items[2], reinterpret_cast<void *>(turn_off_all));
	set_item_userptr(general_items[3], reinterpret_cast<void *>(turn));
	set_item_userptr(general_items[4], reinterpret_cast<void *>(set_speed));
	set_item_userptr(general_items[5], reinterpret_cast<void *>(exit_client));
	menus[0] = new_menu((ITEM * *) general_items);
	windows[0] = newwin(30,50,0,0);
	keypad(windows[0], TRUE);
	set_menu_win(menus[0], windows[0]);
	set_menu_sub(menus[0], derwin(windows[0], 10, 45, 3, 2));
	box(windows[0],0,0);
	post_menu(menus[0]);
	wrefresh(windows[0]);
	panels[0] = new_panel(windows[0]);
	panels_id.insert(std::make_pair(panels[0], 0));
	gen_funcs[DOWN_COMMAND] = down_general_menu;
	gen_funcs[UP_COMMAND] = up_general_menu;
	gen_funcs[CHOOSE_COMMAND] = choose_general_menu;
	set_panel_userptr(panels[0], reinterpret_cast<void *>(gen_funcs));
	
	// Trains menu
	ITEM * * train_turn_items = (ITEM **) calloc(n_train_choices + 1, sizeof(ITEM *));
	ITEM * * train_speed_items = (ITEM **) calloc(n_train_choices + 1, sizeof(ITEM *));
	for (int i = 0; i < n_train_choices; ++i) {
		train_turn_items[i] = new_item(train_choices[i], "ON");
		set_item_userptr(train_turn_items[i], reinterpret_cast<void *>(turn_train_menu));
		train_speed_items[i] = new_item(train_choices[i], "100");
		set_item_userptr(train_speed_items[i], reinterpret_cast<void *>(speed_train_menu));
	}
	set_item_userptr(train_turn_items[7], reinterpret_cast<void *>(exit_train_panel));
	set_item_userptr(train_speed_items[7], reinterpret_cast<void *>(exit_train_panel));
	train_turn_items[n_train_choices] = (ITEM *) NULL;
	train_speed_items[n_train_choices] = (ITEM *) NULL;
	menus[1] = new_menu((ITEM * *) train_turn_items);
	menus[2] = new_menu((ITEM * *) train_speed_items);
	windows[1] = newwin(30,50,0,0);
	windows[2] = newwin(30,50,0,0);
	keypad(windows[1], TRUE);
	keypad(windows[2], TRUE);
	set_menu_win(menus[1], windows[1]);
	set_menu_win(menus[2], windows[2]);
	set_menu_sub(menus[1], derwin(windows[1], 10, 45, 3, 2));
	set_menu_sub(menus[2], derwin(windows[2], 10, 45, 3, 2));
	box(windows[1],0,0);
	box(windows[2],0,0);
	post_menu(menus[1]);
	post_menu(menus[2]);
	wrefresh(windows[1]);
	wrefresh(windows[2]);
	panels[1] = new_panel(windows[1]);
	panels[2] = new_panel(windows[2]);
	panels_id.insert(std::make_pair(panels[1], 1));
	panels_id.insert(std::make_pair(panels[2], 2));
	set_panel_userptr(panels[1], train_mturn_funcs);
	set_panel_userptr(panels[2], train_mspeed_funcs);
	train_mturn_funcs[DOWN_COMMAND] = down_general_menu;
	train_mturn_funcs[UP_COMMAND] = up_general_menu;
	train_mturn_funcs[CHOOSE_COMMAND] = choose_general_menu;
	train_mspeed_funcs[DOWN_COMMAND] = down_general_menu;
	train_mspeed_funcs[UP_COMMAND] = up_general_menu;
	train_mspeed_funcs[CHOOSE_COMMAND] = choose_general_menu;

	// Speed window
	windows[3] = newwin(5, 20, (LINES-5)/2, (COLS-20)/2);
	box(windows[3], 0, 0);
	keypad(windows[3], TRUE);
	panels[3] = new_panel(windows[3]);
	train_speed_funcs[DOWN_COMMAND] = nullptr;
	train_speed_funcs[UP_COMMAND] = nullptr;
	train_speed_funcs[CHOOSE_COMMAND] = speed_window_confirm;
	set_panel_userptr(panels[3], train_speed_funcs);
	hide_panel(panels[3]);

	top_panel(panels[0]);

	update_panels();
	doupdate();
	refresh();
	/*------------ Main loop ---------------*/
	while (true) {
		PANEL * top = panel_below((PANEL *)0);

		// Throws update potentiometer thread
		if (command == 10 && top == panels[3]) {
			std::thread tupdatepot (potentiometer_update);
			tupdatepot.detach();
		}

		command = wgetch(panel_window(top));
		void (**funcs)(int) = reinterpret_cast<void (**)(int)>(const_cast<void *>(panel_userptr(top)));
		switch(command) {	
			case KEY_DOWN: {
					if (funcs[DOWN_COMMAND] != nullptr)
						funcs[DOWN_COMMAND](panels_id[top]);
				break;
			}
			case KEY_UP: {
					if (funcs[UP_COMMAND] != nullptr)
						funcs[UP_COMMAND](panels_id[top]);
				break;
		    }
			case 10: {
					if (funcs[CHOOSE_COMMAND] != nullptr)
						funcs[CHOOSE_COMMAND](panels_id[top]);
				break;		 
			}
		}
		if (command == -1) break;

		top = panel_below((PANEL *)0);

		refresh();
		wrefresh(panel_window(top));
		update_panels();
		doupdate();
	}

	endwin();
	return 0;
}