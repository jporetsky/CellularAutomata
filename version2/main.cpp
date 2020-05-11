//
//  main.c
//  Cellular Automaton


 /*-------------------------------------------------------------------------+
 |	A graphic front end for a grid+state simulation.						|
 |																			|
 |	This application simply creates a glut window with a pane to display	|
 |	a colored grid and the other to display some state information.			|
 |	Sets up callback functions to handle menu, mouse and keyboard events.	|
 |	Normally, you shouldn't have to touch anything in this code, unless		|
 |	you want to change some of the things displayed, add menus, etc.		|
 |	Only mess with this after everything else works and making a backup		|
 |	copy of your project.  OpenGL & glut are tricky and it's really easy	|
 |	to break everything with a single line of code.							|
 |																			|
 |	Current keyboard controls:												|
 |																			|
 |		- 'ESC' --> exit the application									|
 |		- space bar --> resets the grid										|
 |																			|
 |		- 'c' --> toggle color mode on/off									|
 |		- 'b' --> toggles color mode off/on									|
 |		- 'l' --> toggles on/off grid line rendering						|
 |																			|
 |		- '+' --> increase simulation speed									|
 |		- '-' --> reduce simulation speed									|
 |																			|
 |		- '1' --> apply Rule 1 (Conway's classical Game of Life: B3/S23)	|
 |		- '2' --> apply Rule 2 (Coral: B3/S45678)							|
 |		- '3' --> apply Rule 3 (Amoeba: B357/S1358)							|
 |		- '4' --> apply Rule 4 (Maze: B3/S12345)							|
 |																			|
 +-------------------------------------------------------------------------*/

#include <cstdio>
#include <cstdlib>
#include <unistd.h>
#include <time.h>
#include <pthread.h>
//
#include "gl_frontEnd.h"

//==================================================================================
//	Custom data types
//==================================================================================
using ThreadInfo = struct {
	pthread_t id;
	unsigned int index;
};


//==================================================================================
//	Function prototypes
//==================================================================================
void displayGridPane(void);
void displayStatePane(void);
void initializeApplication(void);
void* threadFunc(void*);
unsigned int cellNewState(unsigned int i, unsigned int j);
void aquireLocksAbout(unsigned int row, unsigned int col);
void releaseLocksAbout(unsigned int row, unsigned int col);


//==================================================================================
//	Precompiler #define to let us specify how things should be handled at the
//	border of the frame
//==================================================================================

#define FRAME_DEAD		0	//	cell borders are kept dead
#define FRAME_RANDOM	1	//	new random values are generated at each generation
#define FRAME_CLIPPED	2	//	same rule as elsewhere, with clipping to stay within bounds
#define FRAME_WRAP		3	//	same rule as elsewhere, with wrapping around at edges

//	Pick one value for FRAME_BEHAVIOR
#define FRAME_BEHAVIOR	FRAME_DEAD

//==================================================================================
//	Application-level global variables
//==================================================================================

//	Don't touch
extern int GRID_PANE, STATE_PANE;
extern int gMainWindow, gSubwindow[2];

//	The state grid and its dimensions.  We now have two copies of the grid:
//		- grid is the one displayed in the graphic front end
//		- grid is the grid that stores the next generation of cell
//			states, as computed by our threads.

// option 1: seperate arrays of cells and arrays
//unsigned int** grid;
//pthread_mutex_t** cellLock;

// Option 2: group the cell state + lock in a struct
using Cell = struct {
	unsigned int state;
	pthread_mutex_t lock;
};
Cell** cGrid;

//	Piece of advice, whenever you do a grid-based (e.g. image processing),
//	you should always try to run your code with a non-square grid to
//	spot accidental row-col inversion bugs.
const unsigned int NUM_ROWS = 400, NUM_COLS = 420;

//	the number of live threads (that haven't terminated yet)
const unsigned int MAX_NUM_THREADS = 10;
unsigned int numLiveThreads = 0;

unsigned int rule = GAME_OF_LIFE_RULE;
unsigned int speed = 250;

unsigned int colorMode = 0;
//// ds
///dsfhd
/  sgd
///hsdh


//==================================================================================
//	These are the functions that tie the simulation with the rendering.
//	Some parts are "don't touch."  Other parts need your intervention
//	to make sure that access to critical section is properly synchronized
//==================================================================================


void displayGridPane(void)
{
	//	This is OpenGL/glut magic.  Don't touch
	glutSetWindow(gSubwindow[GRID_PANE]);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	//---------------------------------------------------------
	//	This is the call that makes OpenGL render the grid.
	//
	//---------------------------------------------------------
	drawGrid(grid, NUM_ROWS, NUM_COLS);
	
	//	This is OpenGL/glut magic.  Don't touch
	glutSwapBuffers();
	glutSetWindow(gMainWindow);
}

void displayStatePane(void)
{
	//	This is OpenGL/glut magic.  Don't touch
	glutSetWindow(gSubwindow[STATE_PANE]);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	//---------------------------------------------------------
	//	This is the call that makes OpenGL render information
	//	about the state of the simulation.
	//
	//---------------------------------------------------------
	drawState(numLiveThreads);
	
	
	//	This is OpenGL/glut magic.  Don't touch
	glutSwapBuffers();
	glutSetWindow(gMainWindow);
}

//------------------------------------------------------------------------
//	You shouldn't have to change anything in the main function
//------------------------------------------------------------------------
int main(int argc, char** argv)
{
	//	This takes care of initializing glut and the GUI.
	//	You shouldn’t have to touch this
	initializeFrontEnd(argc, argv, displayGridPane, displayStatePane);
	
	//	Now we can do application-level initialization
	initializeApplication();

	//	Now would be the place & time to create mutex locks and threads
	threadInfo = new ThreadInfo[MAX_NUM_THREADS];
	for (unsigned int k = 0; k < MAX_NUM_THREADS; k++) {
		threadInfo[k].index = k; 
	}
	for (unsigned int k = 0; k < MAX_NUM_THREADS; k++) {
		pthread_create(&(threadInfo[k].id),
			nullptr,
			threadFunc,
			threadInfo + k);
	}

	//	Now we enter the main loop of the program and to a large extend
	//	"lose control" over its execution.  The callback functions that 
	//	we set up earlier will be called when the corresponding event
	//	occurs
	glutMainLoop();
	
	//	This will never be executed (the exit point will be in one of the
	//	call back functions).
	return 0;
}


//==================================================================================
//
//	This is a part that you may have to edit and add to.
//
//==================================================================================

void cleanupAndQuit(void)
{
	//	Free allocated resource before leaving (not absolutely needed, but
	//	just nicer.  Also, if you crash there, you know something is wrong
	//	in your code.
	for (unsigned int i=1; i<NUM_ROWS; i++)
	{
		delete []grid[i];
	}
	delete []grid;

	exit(0);
}

///////////////////// CHECK THE CODE BELOW....UNSURE OF SYNTAX
//*******************************************

void initializeApplication(void)
{
	//  Allocate 2D grids
	//--------------------

	//// Option 1
 //   grid = new unsigned int*[NUM_ROWS];
 //   cellLock = new pthread_mutex_t*[NUM_ROWS];
 //   for (unsigned int i=0; i<NUM_ROWS; i++)
 //   {
 //       grid[i] = new unsigned int[NUM_COLS];
	//	cellLock[i] = new pthread_mutex_t[NUM_COLS];
	//	for(unsigned int j = 0; j < NUM_COLS; j++)
	//		pthread_mutex_init(cellLock[i]+j, nullptr)
 //   }

	// Option 2:
	cGrid = new Cell*[NUM_ROWS];
	for (unsigned int i = 0; i < NUM_ROWS; i++) {
		cGrid[i] = new Cell[NUM_COLS];
		for (unsigned int j = 0; j < NUM_COLS; j++)
			pthread_mutex_init(&(cGrid[i][j].lock), nullptr);
	}
	
	//---------------------------------------------------------------
	//	All the code below to be replaced/removed
	//	I initialize the grid's pixels to have something to look at
	//---------------------------------------------------------------
	//	Yes, I am using the C random generator after ranting in class that the C random
	//	generator was junk.  Here I am not using it to produce "serious" data (as in a
	//	simulation), only some color, in meant-to-be-thrown-away code
	
	//	seed the pseudo-random generator
	srand((unsigned int) time(NULL));
	
	resetGrid();
}

//---------------------------------------------------------------------
//	Implement this function
//---------------------------------------------------------------------

void* threadFunc(void* arg)
{
	(void) arg;
	
	bool keepGoing = true;
	while (keepGoing) {
		// pick a random grid cell
		unsigned int row, col;
		#if FRAME_BEHAVIOR == FRAME_DEAD


		#elif FRAME_BEHAVIOR == FRAME_RANDOM


		#elif FRAME_BEHAVIOR == FRAME_CLIPPED


		#elif FRAME_BEHAVIOR == FRAME_WRAPPED

		#endif

		//aquire lock(s) about (row, col) cell
		//***************************************
		aquireLocksAbout(row, col); //////////////////////////vid2-5/9: 8:00
		unsigned int newState = cellNewState(i, j);

		//	In black and white mode, only alive/dead matters
		//	Dead is dead in any mode
		if (colorMode == 0 || newState == 0)
		{
			// option 1
			// grid[row][col] = newState;
			// Option 2
			cGrid[row][col].state = newState
		}
		//	in color mode, color reflext the "age" of a live cell
		else
		{
			//	Any cell that has not yet reached the "very old cell"
			//	stage simply got one generation older
			if (grid[row][col] < NB_COLORS - 1)
				grid[row][col]++;

		}

		// Release lock(s) about (row, col) cell
		releaseLocksAbout(row, col); //////////////////////////vid2-5/9: 8:00
		usleep(speed); // don't need to sleep as long

	}
	return NULL;
}


void resetGrid(void)
{
	for (unsigned int i=0; i<NUM_ROWS; i++)
	{
		for (unsigned int j=0; j<NUM_COLS; j++)
		{
			grid[i][j] = rand() % 2;
		}
	}
}


//	Here I give three different implementations
//	of a slightly different algorithm, allowing for changes at the border
//	All three variants are used for simulations in research applications.
//	I also refer explicitly to the S/B elements of the "rule" in place.
unsigned int cellNewState(unsigned int i, unsigned int j)
{
	//	First count the number of neighbors that are alive
	//----------------------------------------------------
	//	Again, this implementation makes no pretense at being the most efficient.
	//	I am just trying to keep things modular and somewhat readable
	int count = 0;

	//	Away from the border, we simply count how many among the cell's
	//	eight neighbors are alive (cell state > 0)
	if (i>0 && i<NUM_ROWS-1 && j>0 && j<NUM_COLS-1)
	{
		//	remember that in C, (x == val) is either 1 or 0
		count = (grid[i-1][j-1] != 0) +
				(grid[i-1][j] != 0) +
				(grid[i-1][j+1] != 0)  +
				(grid[i][j-1] != 0)  +
				(grid[i][j+1] != 0)  +
				(grid[i+1][j-1] != 0)  +
				(grid[i+1][j] != 0)  +
				(grid[i+1][j+1] != 0);
	}
	//	on the border of the frame...
	else
	{
		#if FRAME_BEHAVIOR == FRAME_DEAD
		
			//	Hack to force death of a cell
			count = -1;
		
		#elif FRAME_BEHAVIOR == FRAME_RANDOM
		
			count = rand() % 9;
		
		#elif FRAME_BEHAVIOR == FRAME_CLIPPED
	
			if (i>0)
			{
				if (j>0 && grid[i-1][j-1] != 0)
					count++;
				if (grid[i-1][j] != 0)
					count++;
				if (j<NUM_COLS-1 && grid[i-1][j+1] != 0)
					count++;
			}

			if (j>0 && grid[i][j-1] != 0)
				count++;
			if (j<NUM_COLS-1 && grid[i][j+1] != 0)
				count++;

			if (i<NUM_ROWS-1)
			{
				if (j>0 && grid[i+1][j-1] != 0)
					count++;
				if (grid[i+1][j] != 0)
					count++;
				if (j<NUM_COLS-1 && grid[i+1][j+1] != 0)
					count++;
			}
			
	
		#elif FRAME_BEHAVIOR == FRAME_WRAPPED
	
			unsigned int 	iM1 = (i+NUM_ROWS-1)%NUM_ROWS,
							iP1 = (i+1)%NUM_ROWS,
							jM1 = (j+NUM_COLS-1)%NUM_COLS,
							jP1 = (j+1)%NUM_COLS;
			count = grid[iM1][jM1] != 0 +
					grid[iM1][j] != 0 +
					grid[iM1][jP1] != 0  +
					grid[i][jM1] != 0  +
					grid[i][jP1] != 0  +
					grid[iP1][jM1] != 0  +
					grid[iP1][j] != 0  +
					grid[iP1][jP1] != 0 ;

		#else
			#error undefined frame behavior
		#endif
		
	}	//	end of else case (on border)
	
	//	Next apply the cellular automaton rule
	//----------------------------------------------------
	//	by default, the grid square is going to be empty/dead
	unsigned int newState = 0;
	
	//	unless....
	
	switch (rule)
	{
		//	Rule 1 (Conway's classical Game of Life: B3/S23)
		case GAME_OF_LIFE_RULE:

			//	if the cell is currently occupied by a live cell, look at "Stay alive rule"
			if (grid[i][j] != 0)
			{
				if (count == 3 || count == 2)
					newState = 1;
			}
			//	if the grid square is currently empty, look at the "Birth of a new cell" rule
			else
			{
				if (count == 3)
					newState = 1;
			}
			break;
	
		//	Rule 2 (Coral Growth: B3/S45678)
		case CORAL_GROWTH_RULE:

			//	if the cell is currently occupied by a live cell, look at "Stay alive rule"
			if (grid[i][j] != 0)
			{
				if (count > 3)
					newState = 1;
			}
			//	if the grid square is currently empty, look at the "Birth of a new cell" rule
			else
			{
				if (count == 3)
					newState = 1;
			}
			break;
			
		//	Rule 3 (Amoeba: B357/S1358)
		case AMOEBA_RULE:

			//	if the cell is currently occupied by a live cell, look at "Stay alive rule"
			if (grid[i][j] != 0)
			{
				if (count == 1 || count == 3 || count == 5 || count == 8)
					newState = 1;
			}
			//	if the grid square is currently empty, look at the "Birth of a new cell" rule
			else
			{
				if (count == 1 || count == 3 || count == 5 || count == 8)
					newState = 1;
			}
			break;
		
		//	Rule 4 (Maze: B3/S12345)							|
		case MAZE_RULE:

			//	if the cell is currently occupied by a live cell, look at "Stay alive rule"
			if (grid[i][j] != 0)
			{
				if (count >= 1 && count <= 5)
					newState = 1;
			}
			//	if the grid square is currently empty, look at the "Birth of a new cell" rule
			else
			{
				if (count == 3)
					newState = 1;
			}
			break;

			break;
		
		default:
			printf("Invalid rule number\n");
			exit(5);
	}

	return newState;
}

void aquireLocksAbout(unsigned int row, unsigned int col) {
	#if FRAME_BEHAVIOR == FRAME_DEAD
		for (int i = -1; i < 1; i++)
			for (int j = -1; j < 1; j++)
				pthread_mutex_lock(&(cellLock[row+i][col+j]));

	#elif FRAME_BEHAVIOR == FRAME_RANDOM
	pthread_mutex_lock(cellLock[row] + col);

	#elif FRAME_BEHAVIOR == FRAME_CLIPPED


	#elif FRAME_BEHAVIOR == FRAME_WRAPPED
		for (int i = -1; i < 1; i++)
			for (int j = -1; j < 1; j++)
				pthread_mutex_lock(&(cellLock[(row + NUM_ROWS + i) % NUM_ROWS][(col + NUM_COLS + j) % NUM_COLS]));

	#endif
}
void releaseLocksAbout(unsigned int row, unsigned int col) {
	#if FRAME_BEHAVIOR == FRAME_DEAD


	#elif FRAME_BEHAVIOR == FRAME_RANDOM


	#elif FRAME_BEHAVIOR == FRAME_CLIPPED


	#elif FRAME_BEHAVIOR == FRAME_WRAPPED

	#endif
}