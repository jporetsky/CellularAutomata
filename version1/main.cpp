//
//  main.c
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
using ThreadInfo = struct {
	pthread_t id;
	unsigned int index;
	unsigned int startRow, endRow;
	pthread_mutex_t lock;
};


//==================================================================================
//	Function prototypes
//==================================================================================
void displayGridPane(void);
void displayStatePane(void);
void initializeApplication(void);
void* threadFunc(void*);
void swapGrids(void);
unsigned int cellNewState(unsigned int i, unsigned int j);
void createThreads(void);

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
//		- currentGrid is the one displayed in the graphic front end
//		- nextGrid is the grid that stores the next generation of cell
//			states, as computed by our threads.
unsigned int** currentGrid;
unsigned int** nextGrid;

//	Piece of advice, whenever you do a grid-based (e.g. image processing),
//	you should always try to run your code with a non-square grid to
//	spot accidental row-col inversion bugs.
unsigned int numRows = 400, numCols = 420;

//	the number of live threads (that haven't terminated yet)
unsigned int maxNumThreads = 10;
unsigned int numLiveThreads = 0;

unsigned int rule = GAME_OF_LIFE_RULE;
unsigned int speed = 5000;

unsigned int colorMode = 0;

ThreadInfo* threadInfo;

int generation = 0;

unsigned int threadsDoneCount = 0;
pthread_mutex_t threadCountLock;

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
	drawGrid(currentGrid, numRows, numCols);
	
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
int main(int argc, char** argv) {
	if (argc != 4) {
		fprintf(stderr, "cell program launched with incorrect number of arguments.\n"
			"Proper usage: ./cell [number of rows] [number of cols] [max number of live threads]\n"
			"Suggested values: rows: 400, cols: 420, threads: 10\n");
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
	createThreads();
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
    for (unsigned int i=1; i<numRows; i++)
    {
        delete []currentGrid[i];
        delete []nextGrid[i];
    }
	delete []currentGrid;
	delete []nextGrid;

	exit(0);
}



void initializeApplication(void)
{
    //  Allocate 2D grids
    //--------------------
    currentGrid = new unsigned int*[numRows];
    nextGrid = new unsigned int*[numRows];
    for (unsigned int i=0; i<numRows; i++)
    {
        currentGrid[i] = new unsigned int[numCols];
        nextGrid[i] = new unsigned int[numCols];
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
	ThreadInfo* info = static_cast<ThreadInfo*>(arg);
	
	bool keepGoing = true;
	while (keepGoing) {
		//std::cout << "startrow: " << info << std::endl;
		for (unsigned int i = info->startRow; i <= info->endRow; i++)
		{
			for (unsigned int j = 0; j < numCols; j++)
			{
				unsigned int newState = cellNewState(i, j);

				//	In black and white mode, only alive/dead matters
				//	Dead is dead in any mode
				if (colorMode == 0 || newState == 0) {
					nextGrid[i][j] = newState;
				}
				//	in color mode, color reflext the "age" of a live cell
				else {
					//	Any cell that has not yet reached the "very old cell"
					//	stage simply got one generation older
					if (currentGrid[i][j] < NB_COLORS - 1)
						nextGrid[i][j] = currentGrid[i][j] + 1;
					//	An old cell remains old until it dies
					else
						nextGrid[i][j] = currentGrid[i][j];
				}
			}
		}
		// I am done for this generation
		pthread_mutex_lock(&threadCountLock);
		threadsDoneCount++;

		// Check if last thread to finish load
		if (threadsDoneCount == maxNumThreads) {
			pthread_mutex_unlock(&threadCountLock);
			// Can only be done by the last thread to finish its load
			swapGrids();
			usleep(speed);
			threadsDoneCount = 0;
			generation++;  //? not T 04:42
			//threadsDoneCount = 0; // reset to 0 ????

			// wake up the other threads
			for (unsigned int k = 0; k < maxNumThreads; k++) {
				if (k != info->index)
					pthread_mutex_unlock(&(threadInfo[k].lock));
			}
		}
		else {
			pthread_mutex_unlock(&threadCountLock);
			// and wait for the rest of the threads to finish
			pthread_mutex_lock(&(info->lock));
		}
	}
	return nullptr;
}


void resetGrid(void)
{
	for (unsigned int i=0; i<numRows; i++)
	{
		for (unsigned int j=0; j<numCols; j++)
		{
			nextGrid[i][j] = rand() % 2;
		}
	}
	swapGrids();
}

//	This function swaps the current and next grids, as well as their
//	companion 2D grid.  Note that we only swap the "top" layer of
//	the 2D grids.
void swapGrids(void)
{
	unsigned int** tempGrid = currentGrid;
	currentGrid = nextGrid;
	nextGrid = tempGrid;
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
	if (i>0 && i<numRows-1 && j>0 && j<numCols-1)
	{
		//	remember that in C, (x == val) is either 1 or 0
		count = (currentGrid[i-1][j-1] != 0) +
				(currentGrid[i-1][j] != 0) +
				(currentGrid[i-1][j+1] != 0)  +
				(currentGrid[i][j-1] != 0)  +
				(currentGrid[i][j+1] != 0)  +
				(currentGrid[i+1][j-1] != 0)  +
				(currentGrid[i+1][j] != 0)  +
				(currentGrid[i+1][j+1] != 0);
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
				if (j>0 && currentGrid[i-1][j-1] != 0)
					count++;
				if (currentGrid[i-1][j] != 0)
					count++;
				if (j<NUM_COLS-1 && currentGrid[i-1][j+1] != 0)
					count++;
			}

			if (j>0 && currentGrid[i][j-1] != 0)
				count++;
			if (j<NUM_COLS-1 && currentGrid[i][j+1] != 0)
				count++;

			if (i<NUM_ROWS-1)
			{
				if (j>0 && currentGrid[i+1][j-1] != 0)
					count++;
				if (currentGrid[i+1][j] != 0)
					count++;
				if (j<NUM_COLS-1 && currentGrid[i+1][j+1] != 0)
					count++;
			}
			
	
		#elif FRAME_BEHAVIOR == FRAME_WRAPPED
	
			unsigned int 	iM1 = (i+NUM_ROWS-1)%NUM_ROWS,
							iP1 = (i+1)%NUM_ROWS,
							jM1 = (j+NUM_COLS-1)%NUM_COLS,
							jP1 = (j+1)%NUM_COLS;
			count = currentGrid[iM1][jM1] != 0 +
					currentGrid[iM1][j] != 0 +
					currentGrid[iM1][jP1] != 0  +
					currentGrid[i][jM1] != 0  +
					currentGrid[i][jP1] != 0  +
					currentGrid[iP1][jM1] != 0  +
					currentGrid[iP1][j] != 0  +
					currentGrid[iP1][jP1] != 0 ;

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
			if (currentGrid[i][j] != 0)
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
			if (currentGrid[i][j] != 0)
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
			if (currentGrid[i][j] != 0)
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
			if (currentGrid[i][j] != 0)
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

void createThreads(void) {
	pthread_mutex_init(&threadCountLock, nullptr);
	// initialize array of ThreadInfo structs
	threadInfo = new ThreadInfo[maxNumThreads];
	
	unsigned int p = numRows / maxNumThreads;
	unsigned int m = numRows % maxNumThreads; // threads with +1 load
	unsigned int startRow = 0;
	for (unsigned int k = 0; k < maxNumThreads; k++) {
		threadInfo[k].index = k;
		
		unsigned int endRow = k < m ? startRow + p : startRow + p - 1;

		// Compute startRow, endRow
		threadInfo[k].startRow = startRow;
		threadInfo[k].endRow = endRow;
		startRow = endRow + 1;

		// Create the lock pre-locked. Don't think there's another way to do this.
		pthread_mutex_init(&(threadInfo[k].lock), nullptr);
		pthread_mutex_lock(&(threadInfo[k].lock));
	}

	for (unsigned int k = 0; k < maxNumThreads; k++) {
		// create thread k
		int error_code = pthread_create(&(threadInfo[k].id),	// ptr to pthread_t
										nullptr, 				// thread attributes
										threadFunc,				// thread function
										&(threadInfo[k]));		// pointer to the data
		if (error_code < 0)
			std::cerr << "ERROR: Failed to create ghost thread with error code " << error_code << std::endl;
		else numLiveThreads++;  // increment counter to display on GUI
	}
}