/*
    gm_navigation
    By Spacetech
*/

#ifndef DEFINES_H
#define DEFINES_H

//#define FILEBUG

#define FILEBUG_WRITE(...) if(pDebugFile != NULL) { fprintf(pDebugFile, __VA_ARGS__); fflush(pDebugFile); }

// Version for save files
#define NAV_VERSION 1

#define NAV_NAME "Nav"
#define NAV_TYPE 156

#define NODE_NAME "Node"
#define NODE_TYPE 157

enum NavDirType
{
	NORTH = 0,
	SOUTH = 1,
	EAST = 2,
	WEST = 3,
	NORTHEAST = 4,
	NORTHWEST = 5,
	SOUTHEAST = 6,
	SOUTHWEST = 7,

	// I'm imagining it like I'm looking straight at it, up being the z direction, which increases in the up direction ^^
	UP = 8,
	DOWN = 9,
	LEFT = 10,
	RIGHT = 11,
	FORWARD = 12,
	BACKWARD = 13
};

#define NUM_DIRECTIONS 4 // NORTH to WEST
#define NUM_DIRECTIONS_DIAGONAL 8 // NORTH to SOUTHWEST
#define NUM_DIRECTIONS_AIR 6 // UP to BACKWARD

// Sue me for thinking about the future and how SOUTHWESTNORTHEAST is a direction!
#define NUM_DIRECTIONS_MAX 14

#endif
