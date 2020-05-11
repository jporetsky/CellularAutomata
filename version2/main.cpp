//
//  main.cpp
//  Cellular Automaton

/****************************************************************************
 * Group members:															*
 *		Shane Wallerick														*
 *		Brendan Tierney														*
 *		Jake Poretsky														*
 *																			*
 * Help was recieved from Professor Jean-Yves Hervé during office hours.	*
 *																			*
 ***************************************************************************/

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

#include <iostream>
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
using ThreadInfo = struct
{
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
#define FRAME_WRAPPED	3	//	same rule as elsewhere, with wrapping around at edges

//	Pick one value for FRAME_BEHAVIOR
#define FRAME_BEHAVIOR	FRAME_DEAD

//==================================================================================
//	Application-level global variables
//==================================================================================

//	Don't touch
extern int GRID_PANE, STATE_PANE;
extern int gMainWindow, gSubwindow[2];

//	The state grid and its dimensions.  We now have two copies of the grid:
// We have 2 separate 2D arrays, one for state, one for locks //remove
unsigned int** grid;
pthread_mutex_t** cellLock;

//	Piece of advice, whenever you do a grid-based (e.g. image processing),
//	you should always try to run your code with a non-square grid to
//	spot accidental row-col inversion bugs.
unsigned int numRows, numCols;

//	the number of live threads (that haven't terminated yet)
unsigned int maxNumThreads;
unsigned int numLiveThreads = 0;

unsigned int rule = GAME_OF_LIFE_RULE;
unsigned int speed = 250; // Intentionally lower than v1

unsigned int colorMode = 0;

ThreadInfo* threadInfo;


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
	drawGrid(grid, numRows, numCols);

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


int main(int argc, char** argv) {
	if (argc != 4) {
		fprintf(stderr, "cell v2 program launched with incorrect number of arguments.\n"
			"Proper usage: ./cell [number of rows] [number of cols] [max number of live threads]\n");
		exit(1);
	}
	numRows = (unsigned int)strtoul(argv[1], NULL, 10);
	numCols = (unsigned int)strtoul(argv[2], NULL, 10);
	maxNumThreads = (unsigned int)strtoul(argv[3], NULL, 10);

	//	This takes care of initializing glut and the GUI.
	//	You shouldn’t have to touch this
	initializeFrontEnd(argc, argv, displayGridPane, displayStatePane);

	//	Now we can do application-level initialization
	initializeApplication();

	//	Now would be the place & time to create mutex locks and threads
	threadInfo = new ThreadInfo[maxNumThreads];
	for (unsigned int k = 0; k < maxNumThreads; k++) {
		threadInfo[k].index = k;
	}
	for (unsigned int k = 0; k < maxNumThreads; k++) {
		int error_code = pthread_create(&(threadInfo[k].id),
										nullptr,
										threadFunc,
										threadInfo + k);
		if (error_code < 0)
			std::cerr << "ERROR: Failed to create ghost thread with error code " << error_code << std::endl;
		else numLiveThreads++;  // increment counter to display on GUI
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


void cleanupAndQuit(void)
{
	//	Free allocated resource before leaving (not absolutely needed, but
	//	just nicer.  Also, if you crash there, you know something is wrong
	//	in your code.
	for (unsigned int i = 1; i < numRows; i++)
	{
		delete[]grid[i];
	}
	delete[]grid;

	exit(0);
}


void initializeApplication(void)
{
	//  Allocate 2D grids
	//--------------------
	grid = new unsigned int* [numRows];
	cellLock = new pthread_mutex_t * [numRows];
	for (unsigned int i = 0; i < numRows; i++) {
		grid[i] = new unsigned int[numCols];
		cellLock[i] = new pthread_mutex_t[numCols];
		for (unsigned int j = 0; j < numCols; j++) {
			// don't create them pre-locked
			pthread_mutex_init(cellLock[i] + j, nullptr);
		}
	}

	//---------------------------------------------------------------
	//	All the code below to be replaced/removed
	//	I initialize the grid's pixels to have something to look at
	//---------------------------------------------------------------
	//	Yes, I am using the C random generator after ranting in class that the C random
	//	generator was junk.  Here I am not using it to produce "serious" data (as in a
	//	simulation), only some color, in meant-to-be-thrown-away code

	//	seed the pseudo-random generator
	srand((unsigned int)time(NULL));

	resetGrid();
}


void* threadFunc(void* arg)
{
	(void)arg;

	bool keepGoing = true;
	while (keepGoing) {
		// pick a random grid cell
		unsigned int row, col;

		row = rand() % numRows;
		col = rand() % numCols;

		//aquire lock(s) about (row, col) cell
		aquireLocksAbout(row, col);
		unsigned int newState = cellNewState(row, col);

		//	In black and white mode, only alive/dead matters
		//	Dead is dead in any mode
		if (colorMode == 0 || newState == 0)
		{
			grid[row][col] = newState;
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
		releaseLocksAbout(row, col);

		usleep(speed);
	}
	return NULL;
}

void resetGrid(void)
{
	for (unsigned int i = 0; i < numRows; i++)
	{
		for (unsigned int j = 0; j < numCols; j++)
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
	if (i > 0 && i < numRows - 1 && j>0 && j < numCols - 1)
	{
		//	remember that in C, (x == val) is either 1 or 0
		count = (grid[i - 1][j - 1] != 0) +
			(grid[i - 1][j] != 0) +
			(grid[i - 1][j + 1] != 0) +
			(grid[i][j - 1] != 0) +
			(grid[i][j + 1] != 0) +
			(grid[i + 1][j - 1] != 0) +
			(grid[i + 1][j] != 0) +
			(grid[i + 1][j + 1] != 0);
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

		if (i > 0)
		{
			if (j > 0 && grid[i - 1][j - 1] != 0)
				count++;
			if (grid[i - 1][j] != 0)
				count++;
			if (j < numCols - 1 && grid[i - 1][j + 1] != 0)
				count++;
		}

		if (j > 0 && grid[i][j - 1] != 0)
			count++;
		if (j < numCols - 1 && grid[i][j + 1] != 0)
			count++;

		if (i < numRows - 1)
		{
			if (j > 0 && grid[i + 1][j - 1] != 0)
				count++;
			if (grid[i + 1][j] != 0)
				count++;
			if (j < numCols - 1 && grid[i + 1][j + 1] != 0)
				count++;
		}


#elif FRAME_BEHAVIOR == FRAME_WRAPPED

		unsigned int 	iM1 = (i + numRows - 1) % numRows,
			iP1 = (i + 1) % numRows,
			jM1 = (j + numCols - 1) % numCols,
			jP1 = (j + 1) % numCols;
		count = grid[iM1][jM1] != 0 +
			grid[iM1][j] != 0 +
			grid[iM1][jP1] != 0 +
			grid[i][jM1] != 0 +
			grid[i][jP1] != 0 +
			grid[iP1][jM1] != 0 +
			grid[iP1][j] != 0 +
			grid[iP1][jP1] != 0;

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
	// If the cell is not on the border, we lock all 9 cells
	if (row > 0 && row < numRows - 1 && col>0 && col < numCols - 1)
		for (int i = -1; i < +1; i++)
			for (int j = -1; j < +1; j++)
				pthread_mutex_lock(&(cellLock[row + i][col + j]));
	// Else, on border neighbors are ignored (cell stays dead)
	else
		pthread_mutex_lock(&(cellLock[row][col]));

#elif FRAME_BEHAVIOR == FRAME_RANDOM
	pthread_mutex_lock(cellLock[row] + col);

#elif FRAME_BEHAVIOR == FRAME_CLIPPED
	// increasing order for rows and columns
	for (int i = -1; i < +1; i++)
		for (int j = -1; j < +1; j++)
			// if row+i and col+j are within the grid
			if(row+k >= 0 && row+i < numRows && col + j >= 0 && col + j < numCols)
				pthread_mutex_lock(&(cellLock[row + i][col + j]));


#elif FRAME_BEHAVIOR == FRAME_WRAP
	for (int i = -1; i < +1; i++)
		for (int j = -1; j < +1; j++)
			pthread_mutex_lock(&(cellLock[(row + numRows + i) % numRows][(col + numCols + j) % numCols]));

	#endif
}
void releaseLocksAbout(unsigned int row, unsigned int col) {
#if FRAME_BEHAVIOR == FRAME_DEAD
	// If the cell is not on the border, we lock all nine cells
	if (row > 0 && row < numRows - 1 && col>0 && col < numCols - 1)
		for (int i = -1; i < 1; i++)
			for (int j = -1; j < 1; j++)
				pthread_mutex_unlock(&(cellLock[row + i][col + j]));
	// else, on border, neighbors are ignored (cell stays dead)
	else
		pthread_mutex_unlock(&(cellLock[row][col]));

#elif FRAME_BEHAVIOR == FRAME_RANDOM
	pthread_mutex_unlock(cellLock[row] + col);

#elif FRAME_BEHAVIOR == FRAME_CLIPPED
	// increasing order for rows and columns
	for (int i = -1; i < +1; i++)
		for (int j = -1; j < +1; j++)
			// if row+i and col+j are within the grid
			if (row + k >= 0 && row + i < numRows && col + j >= 0 && col + j < numCols)
				pthread_mutex_lock(&(cellLock[row + i][col + j]));

#elif FRAME_BEHAVIOR == FRAME_WRAPPED
	for (int i = -1; i < +1; i++)
		for (int j = -1; j < +1; j++)
			pthread_mutex_unlock(&(cellLock[(row + numRows + i) % numRows][(col + numCols + j) % numCols]));

#endif
}