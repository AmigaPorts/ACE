/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

// Info regarding MOD file format:
// - https://www.fileformat.info/format/mod/corion.htm
// - http://lclevy.free.fr/mo3/mod.txt

#include <ace/utils/ptplayer.h>
#include <ace/utils/custom.h>
#include <hardware/intbits.h>
#include <hardware/dmabits.h>

static UBYTE MasterVolTab[][65] = {
	{
		0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
		0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
		0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
		0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
		0
	}, {
		0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
		0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
		0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
		0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
		1
	}, {
		0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
		0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
		1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
		1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
		2
	}, {
		0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
		0,0,0,0,0,0,1,1,1,1,1,1,1,1,1,1,
		1,1,1,1,1,1,1,1,1,1,1,2,2,2,2,2,
		2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
		3
	}, {
		0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
		1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
		2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
		3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,
		4
	}, {
		0,0,0,0,0,0,0,0,0,0,0,0,0,1,1,1,
		1,1,1,1,1,1,1,1,1,1,2,2,2,2,2,2,
		2,2,2,2,2,2,2,3,3,3,3,3,3,3,3,3,
		3,3,3,3,4,4,4,4,4,4,4,4,4,4,4,4,
		5
	}, {
		0,0,0,0,0,0,0,0,0,0,0,1,1,1,1,1,
		1,1,1,1,1,1,2,2,2,2,2,2,2,2,2,2,
		3,3,3,3,3,3,3,3,3,3,3,4,4,4,4,4,
		4,4,4,4,4,4,5,5,5,5,5,5,5,5,5,5,
		6
	}, {
		0,0,0,0,0,0,0,0,0,0,1,1,1,1,1,1,
		1,1,1,2,2,2,2,2,2,2,2,2,3,3,3,3,
		3,3,3,3,3,4,4,4,4,4,4,4,4,4,5,5,
		5,5,5,5,5,5,5,6,6,6,6,6,6,6,6,6,
		7
	}, {
		0,0,0,0,0,0,0,0,1,1,1,1,1,1,1,1,
		2,2,2,2,2,2,2,2,3,3,3,3,3,3,3,3,
		4,4,4,4,4,4,4,4,5,5,5,5,5,5,5,5,
		6,6,6,6,6,6,6,6,7,7,7,7,7,7,7,7,
		8
	}, {
		0,0,0,0,0,0,0,0,1,1,1,1,1,1,1,2,
		2,2,2,2,2,2,3,3,3,3,3,3,3,4,4,4,
		4,4,4,4,5,5,5,5,5,5,5,6,6,6,6,6,
		6,6,7,7,7,7,7,7,7,8,8,8,8,8,8,8,
		9
	}, {
		0,0,0,0,0,0,0,1,1,1,1,1,1,2,2,2,
		2,2,2,2,3,3,3,3,3,3,4,4,4,4,4,4,
		5,5,5,5,5,5,5,6,6,6,6,6,6,7,7,7,
		7,7,7,7,8,8,8,8,8,8,9,9,9,9,9,9,
		10
	}, {
		0,0,0,0,0,0,1,1,1,1,1,1,2,2,2,2,
		2,2,3,3,3,3,3,3,4,4,4,4,4,4,5,5,
		5,5,5,6,6,6,6,6,6,7,7,7,7,7,7,8,
		8,8,8,8,8,9,9,9,9,9,9,10,10,10,10,10,
		11
	}, {
		0,0,0,0,0,0,1,1,1,1,1,2,2,2,2,2,
		3,3,3,3,3,3,4,4,4,4,4,5,5,5,5,5,
		6,6,6,6,6,6,7,7,7,7,7,8,8,8,8,8,
		9,9,9,9,9,9,10,10,10,10,10,11,11,11,11,11,
		12
	}, {
		0,0,0,0,0,1,1,1,1,1,2,2,2,2,2,3,
		3,3,3,3,4,4,4,4,4,5,5,5,5,5,6,6,
		6,6,6,7,7,7,7,7,8,8,8,8,8,9,9,9,
		9,9,10,10,10,10,10,11,11,11,11,11,12,12,12,12,
		13
	}, {
		0,0,0,0,0,1,1,1,1,1,2,2,2,2,3,3,
		3,3,3,4,4,4,4,5,5,5,5,5,6,6,6,6,
		7,7,7,7,7,8,8,8,8,8,9,9,9,9,10,10,
		10,10,10,11,11,11,11,12,12,12,12,12,13,13,13,13,
		14
	}, {
		0,0,0,0,0,1,1,1,1,2,2,2,2,3,3,3,
		3,3,4,4,4,4,5,5,5,5,6,6,6,6,7,7,
		7,7,7,8,8,8,8,9,9,9,9,10,10,10,10,11,
		11,11,11,11,12,12,12,12,13,13,13,13,14,14,14,14,
		15
	}, {
		0,0,0,0,1,1,1,1,2,2,2,2,3,3,3,3,
		4,4,4,4,5,5,5,5,6,6,6,6,7,7,7,7,
		8,8,8,8,9,9,9,9,10,10,10,10,11,11,11,11,
		12,12,12,12,13,13,13,13,14,14,14,14,15,15,15,15,
		16
	}, {
		0,0,0,0,1,1,1,1,2,2,2,2,3,3,3,3,
		4,4,4,5,5,5,5,6,6,6,6,7,7,7,7,8,
		8,8,9,9,9,9,10,10,10,10,11,11,11,11,12,12,
		12,13,13,13,13,14,14,14,14,15,15,15,15,16,16,16,
		17
	}, {
		0,0,0,0,1,1,1,1,2,2,2,3,3,3,3,4,
		4,4,5,5,5,5,6,6,6,7,7,7,7,8,8,8,
		9,9,9,9,10,10,10,10,11,11,11,12,12,12,12,13,
		13,13,14,14,14,14,15,15,15,16,16,16,16,17,17,17,
		18
	}, {
		0,0,0,0,1,1,1,2,2,2,2,3,3,3,4,4,
		4,5,5,5,5,6,6,6,7,7,7,8,8,8,8,9,
		9,9,10,10,10,10,11,11,11,12,12,12,13,13,13,13,
		14,14,14,15,15,15,16,16,16,16,17,17,17,18,18,18,
		19
	}, {
		0,0,0,0,1,1,1,2,2,2,3,3,3,4,4,4,
		5,5,5,5,6,6,6,7,7,7,8,8,8,9,9,9,
		10,10,10,10,11,11,11,12,12,12,13,13,13,14,14,14,
		15,15,15,15,16,16,16,17,17,17,18,18,18,19,19,19,
		20
	}, {
		0,0,0,0,1,1,1,2,2,2,3,3,3,4,4,4,
		5,5,5,6,6,6,7,7,7,8,8,8,9,9,9,10,
		10,10,11,11,11,12,12,12,13,13,13,14,14,14,15,15,
		15,16,16,16,17,17,17,18,18,18,19,19,19,20,20,20,
		21
	}, {
		0,0,0,1,1,1,2,2,2,3,3,3,4,4,4,5,
		5,5,6,6,6,7,7,7,8,8,8,9,9,9,10,10,
		11,11,11,12,12,12,13,13,13,14,14,14,15,15,15,16,
		16,16,17,17,17,18,18,18,19,19,19,20,20,20,21,21,
		22
	}, {
		0,0,0,1,1,1,2,2,2,3,3,3,4,4,5,5,
		5,6,6,6,7,7,7,8,8,8,9,9,10,10,10,11,
		11,11,12,12,12,13,13,14,14,14,15,15,15,16,16,16,
		17,17,17,18,18,19,19,19,20,20,20,21,21,21,22,22,
		23
	}, {
		0,0,0,1,1,1,2,2,3,3,3,4,4,4,5,5,
		6,6,6,7,7,7,8,8,9,9,9,10,10,10,11,11,
		12,12,12,13,13,13,14,14,15,15,15,16,16,16,17,17,
		18,18,18,19,19,19,20,20,21,21,21,22,22,22,23,23,
		24
	}, {
		0,0,0,1,1,1,2,2,3,3,3,4,4,5,5,5,
		6,6,7,7,7,8,8,8,9,9,10,10,10,11,11,12,
		12,12,13,13,14,14,14,15,15,16,16,16,17,17,17,18,
		18,19,19,19,20,20,21,21,21,22,22,23,23,23,24,24,
		25
	}, {
		0,0,0,1,1,2,2,2,3,3,4,4,4,5,5,6,
		6,6,7,7,8,8,8,9,9,10,10,10,11,11,12,12,
		13,13,13,14,14,15,15,15,16,16,17,17,17,18,18,19,
		19,19,20,20,21,21,21,22,22,23,23,23,24,24,25,25,
		26
	}, {
		0,0,0,1,1,2,2,2,3,3,4,4,5,5,5,6,
		6,7,7,8,8,8,9,9,10,10,10,11,11,12,12,13,
		13,13,14,14,15,15,16,16,16,17,17,18,18,18,19,19,
		20,20,21,21,21,22,22,23,23,24,24,24,25,25,26,26,
		27
	}, {
		0,0,0,1,1,2,2,3,3,3,4,4,5,5,6,6,
		7,7,7,8,8,9,9,10,10,10,11,11,12,12,13,13,
		14,14,14,15,15,16,16,17,17,17,18,18,19,19,20,20,
		21,21,21,22,22,23,23,24,24,24,25,25,26,26,27,27,
		28
	}, {
		0,0,0,1,1,2,2,3,3,4,4,4,5,5,6,6,
		7,7,8,8,9,9,9,10,10,11,11,12,12,13,13,14,
		14,14,15,15,16,16,17,17,18,18,19,19,19,20,20,21,
		21,22,22,23,23,24,24,24,25,25,26,26,27,27,28,28,
		29
	}, {
		0,0,0,1,1,2,2,3,3,4,4,5,5,6,6,7,
		7,7,8,8,9,9,10,10,11,11,12,12,13,13,14,14,
		15,15,15,16,16,17,17,18,18,19,19,20,20,21,21,22,
		22,22,23,23,24,24,25,25,26,26,27,27,28,28,29,29,
		30
	}, {
		0,0,0,1,1,2,2,3,3,4,4,5,5,6,6,7,
		7,8,8,9,9,10,10,11,11,12,12,13,13,14,14,15,
		15,15,16,16,17,17,18,18,19,19,20,20,21,21,22,22,
		23,23,24,24,25,25,26,26,27,27,28,28,29,29,30,30,
		31
	}, {
		0,0,1,1,2,2,3,3,4,4,5,5,6,6,7,7,
		8,8,9,9,10,10,11,11,12,12,13,13,14,14,15,15,
		16,16,17,17,18,18,19,19,20,20,21,21,22,22,23,23,
		24,24,25,25,26,26,27,27,28,28,29,29,30,30,31,31,
		32
	}, {
		0,0,1,1,2,2,3,3,4,4,5,5,6,6,7,7,
		8,8,9,9,10,10,11,11,12,12,13,13,14,14,15,15,
		16,17,17,18,18,19,19,20,20,21,21,22,22,23,23,24,
		24,25,25,26,26,27,27,28,28,29,29,30,30,31,31,32,
		33
	}, {
		0,0,1,1,2,2,3,3,4,4,5,5,6,6,7,7,
		8,9,9,10,10,11,11,12,12,13,13,14,14,15,15,16,
		17,17,18,18,19,19,20,20,21,21,22,22,23,23,24,24,
		25,26,26,27,27,28,28,29,29,30,30,31,31,32,32,33,
		34
	}, {
		0,0,1,1,2,2,3,3,4,4,5,6,6,7,7,8,
		8,9,9,10,10,11,12,12,13,13,14,14,15,15,16,16,
		17,18,18,19,19,20,20,21,21,22,22,23,24,24,25,25,
		26,26,27,27,28,28,29,30,30,31,31,32,32,33,33,34,
		35
	}, {
		0,0,1,1,2,2,3,3,4,5,5,6,6,7,7,8,
		9,9,10,10,11,11,12,12,13,14,14,15,15,16,16,17,
		18,18,19,19,20,20,21,21,22,23,23,24,24,25,25,26,
		27,27,28,28,29,29,30,30,31,32,32,33,33,34,34,35,
		36
	}, {
		0,0,1,1,2,2,3,4,4,5,5,6,6,7,8,8,
		9,9,10,10,11,12,12,13,13,14,15,15,16,16,17,17,
		18,19,19,20,20,21,21,22,23,23,24,24,25,26,26,27,
		27,28,28,29,30,30,31,31,32,32,33,34,34,35,35,36,
		37
	}, {
		0,0,1,1,2,2,3,4,4,5,5,6,7,7,8,8,
		9,10,10,11,11,12,13,13,14,14,15,16,16,17,17,18,
		19,19,20,20,21,21,22,23,23,24,24,25,26,26,27,27,
		28,29,29,30,30,31,32,32,33,33,34,35,35,36,36,37,
		38
	}, {
		0,0,1,1,2,3,3,4,4,5,6,6,7,7,8,9,
		9,10,10,11,12,12,13,14,14,15,15,16,17,17,18,18,
		19,20,20,21,21,22,23,23,24,24,25,26,26,27,28,28,
		29,29,30,31,31,32,32,33,34,34,35,35,36,37,37,38,
		39
	}, {
		0,0,1,1,2,3,3,4,5,5,6,6,7,8,8,9,
		10,10,11,11,12,13,13,14,15,15,16,16,17,18,18,19,
		20,20,21,21,22,23,23,24,25,25,26,26,27,28,28,29,
		30,30,31,31,32,33,33,34,35,35,36,36,37,38,38,39,
		40
	}, {
		0,0,1,1,2,3,3,4,5,5,6,7,7,8,8,9,
		10,10,11,12,12,13,14,14,15,16,16,17,17,18,19,19,
		20,21,21,22,23,23,24,24,25,26,26,27,28,28,29,30,
		30,31,32,32,33,33,34,35,35,36,37,37,38,39,39,40,
		41
	}, {
		0,0,1,1,2,3,3,4,5,5,6,7,7,8,9,9,
		10,11,11,12,13,13,14,15,15,16,17,17,18,19,19,20,
		21,21,22,22,23,24,24,25,26,26,27,28,28,29,30,30,
		31,32,32,33,34,34,35,36,36,37,38,38,39,40,40,41,
		42
	}, {
		0,0,1,2,2,3,4,4,5,6,6,7,8,8,9,10,
		10,11,12,12,13,14,14,15,16,16,17,18,18,19,20,20,
		21,22,22,23,24,24,25,26,26,27,28,28,29,30,30,31,
		32,32,33,34,34,35,36,36,37,38,38,39,40,40,41,42,
		43
	}, {
		0,0,1,2,2,3,4,4,5,6,6,7,8,8,9,10,
		11,11,12,13,13,14,15,15,16,17,17,18,19,19,20,21,
		22,22,23,24,24,25,26,26,27,28,28,29,30,30,31,32,
		33,33,34,35,35,36,37,37,38,39,39,40,41,41,42,43,
		44
	}, {
		0,0,1,2,2,3,4,4,5,6,7,7,8,9,9,10,
		11,11,12,13,14,14,15,16,16,17,18,18,19,20,21,21,
		22,23,23,24,25,26,26,27,28,28,29,30,30,31,32,33,
		33,34,35,35,36,37,37,38,39,40,40,41,42,42,43,44,
		45
	}, {
		0,0,1,2,2,3,4,5,5,6,7,7,8,9,10,10,
		11,12,12,13,14,15,15,16,17,17,18,19,20,20,21,22,
		23,23,24,25,25,26,27,28,28,29,30,30,31,32,33,33,
		34,35,35,36,37,38,38,39,40,40,41,42,43,43,44,45,
		46
	}, {
		0,0,1,2,2,3,4,5,5,6,7,8,8,9,10,11,
		11,12,13,13,14,15,16,16,17,18,19,19,20,21,22,22,
		23,24,24,25,26,27,27,28,29,30,30,31,32,33,33,34,
		35,35,36,37,38,38,39,40,41,41,42,43,44,44,45,46,
		47
	}, {
		0,0,1,2,3,3,4,5,6,6,7,8,9,9,10,11,
		12,12,13,14,15,15,16,17,18,18,19,20,21,21,22,23,
		24,24,25,26,27,27,28,29,30,30,31,32,33,33,34,35,
		36,36,37,38,39,39,40,41,42,42,43,44,45,45,46,47,
		48
	}, {
		0,0,1,2,3,3,4,5,6,6,7,8,9,9,10,11,
		12,13,13,14,15,16,16,17,18,19,19,20,21,22,22,23,
		24,25,26,26,27,28,29,29,30,31,32,32,33,34,35,35,
		36,37,38,39,39,40,41,42,42,43,44,45,45,46,47,48,
		49
	}, {
		0,0,1,2,3,3,4,5,6,7,7,8,9,10,10,11,
		12,13,14,14,15,16,17,17,18,19,20,21,21,22,23,24,
		25,25,26,27,28,28,29,30,31,32,32,33,34,35,35,36,
		37,38,39,39,40,41,42,42,43,44,45,46,46,47,48,49,
		50
	}, {
		0,0,1,2,3,3,4,5,6,7,7,8,9,10,11,11,
		12,13,14,15,15,16,17,18,19,19,20,21,22,23,23,24,
		25,26,27,27,28,29,30,31,31,32,33,34,35,35,36,37,
		38,39,39,40,41,42,43,43,44,45,46,47,47,48,49,50,
		51
	}, {
		0,0,1,2,3,4,4,5,6,7,8,8,9,10,11,12,
		13,13,14,15,16,17,17,18,19,20,21,21,22,23,24,25,
		26,26,27,28,29,30,30,31,32,33,34,34,35,36,37,38,
		39,39,40,41,42,43,43,44,45,46,47,47,48,49,50,51,
		52
	}, {
		0,0,1,2,3,4,4,5,6,7,8,9,9,10,11,12,
		13,14,14,15,16,17,18,19,19,20,21,22,23,24,24,25,
		26,27,28,28,29,30,31,32,33,33,34,35,36,37,38,38,
		39,40,41,42,43,43,44,45,46,47,48,48,49,50,51,52,
		53
	}, {
		0,0,1,2,3,4,5,5,6,7,8,9,10,10,11,12,
		13,14,15,16,16,17,18,19,20,21,21,22,23,24,25,26,
		27,27,28,29,30,31,32,32,33,34,35,36,37,37,38,39,
		40,41,42,43,43,44,45,46,47,48,48,49,50,51,52,53,
		54
	}, {
		0,0,1,2,3,4,5,6,6,7,8,9,10,11,12,12,
		13,14,15,16,17,18,18,19,20,21,22,23,24,24,25,26,
		27,28,29,30,30,31,32,33,34,35,36,36,37,38,39,40,
		41,42,42,43,44,45,46,47,48,48,49,50,51,52,53,54,
		55
	}, {
		0,0,1,2,3,4,5,6,7,7,8,9,10,11,12,13,
		14,14,15,16,17,18,19,20,21,21,22,23,24,25,26,27,
		28,28,29,30,31,32,33,34,35,35,36,37,38,39,40,41,
		42,42,43,44,45,46,47,48,49,49,50,51,52,53,54,55,
		56
	}, {
		0,0,1,2,3,4,5,6,7,8,8,9,10,11,12,13,
		14,15,16,16,17,18,19,20,21,22,23,24,24,25,26,27,
		28,29,30,31,32,32,33,34,35,36,37,38,39,40,40,41,
		42,43,44,45,46,47,48,48,49,50,51,52,53,54,55,56,
		57
	}, {
		0,0,1,2,3,4,5,6,7,8,9,9,10,11,12,13,
		14,15,16,17,18,19,19,20,21,22,23,24,25,26,27,28,
		29,29,30,31,32,33,34,35,36,37,38,38,39,40,41,42,
		43,44,45,46,47,48,48,49,50,51,52,53,54,55,56,57,
		58
	}, {
		0,0,1,2,3,4,5,6,7,8,9,10,11,11,12,13,
		14,15,16,17,18,19,20,21,22,23,23,24,25,26,27,28,
		29,30,31,32,33,34,35,35,36,37,38,39,40,41,42,43,
		44,45,46,47,47,48,49,50,51,52,53,54,55,56,57,58,
		59
	}, {
		0,0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,
		15,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,
		30,30,31,32,33,34,35,36,37,38,39,40,41,42,43,44,
		45,45,46,47,48,49,50,51,52,53,54,55,56,57,58,59,
		60
	}, {
		0,0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,
		15,16,17,18,19,20,20,21,22,23,24,25,26,27,28,29,
		30,31,32,33,34,35,36,37,38,39,40,40,41,42,43,44,
		45,46,47,48,49,50,51,52,53,54,55,56,57,58,59,60,
		61
	}, {
		0,0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,
		15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,
		31,31,32,33,34,35,36,37,38,39,40,41,42,43,44,45,
		46,47,48,49,50,51,52,53,54,55,56,57,58,59,60,61,
		62
	}, {
		0,0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,
		15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,
		31,32,33,34,35,36,37,38,39,40,41,42,43,44,45,46,
		47,48,49,50,51,52,53,54,55,56,57,58,59,60,61,62,
		63
	}, {
		0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,
		16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,
		32,33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,
		48,49,50,51,52,53,54,55,56,57,58,59,60,61,62,63,
		64
	}
};

static const BYTE mt_VibratoSineTable[] = {
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,1,1,1,1,1,1,1,1,1,1,
	1,1,1,1,1,1,1,1,1,1,1,0,0,0,0,0,
	0,0,0,0,0,0,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,0,0,0,0,0,
	0,0,0,1,1,1,2,2,2,3,3,3,3,3,3,3,
	3,3,3,3,3,3,3,3,2,2,2,1,1,1,0,0,
	0,0,0,-1,-1,-1,-2,-2,-2,-3,-3,-3,-3,-3,-3,-3,
	-3,-3,-3,-3,-3,-3,-3,-3,-2,-2,-2,-1,-1,-1,0,0,
	0,0,1,1,2,2,3,3,4,4,4,5,5,5,5,5,
	5,5,5,5,5,5,4,4,4,3,3,2,2,1,1,0,
	0,0,-1,-1,-2,-2,-3,-3,-4,-4,-4,-5,-5,-5,-5,-5,
	-5,-5,-5,-5,-5,-5,-4,-4,-4,-3,-3,-2,-2,-1,-1,0,
	0,0,1,2,3,3,4,5,5,6,6,7,7,7,7,7,
	7,7,7,7,7,7,6,6,5,5,4,3,3,2,1,0,
	0,0,-1,-2,-3,-3,-4,-5,-5,-6,-6,-7,-7,-7,-7,-7,
	-7,-7,-7,-7,-7,-7,-6,-6,-5,-5,-4,-3,-3,-2,-1,0,
	0,0,1,2,3,4,5,6,7,7,8,8,9,9,9,9,
	9,9,9,9,9,8,8,7,7,6,5,4,3,2,1,0,
	0,0,-1,-2,-3,-4,-5,-6,-7,-7,-8,-8,-9,-9,-9,-9,
	-9,-9,-9,-9,-9,-8,-8,-7,-7,-6,-5,-4,-3,-2,-1,0,
	0,1,2,3,4,5,6,7,8,9,9,10,11,11,11,11,
	11,11,11,11,11,10,9,9,8,7,6,5,4,3,2,1,
	0,-1,-2,-3,-4,-5,-6,-7,-8,-9,-9,-10,-11,-11,-11,-11,
	-11,-11,-11,-11,-11,-10,-9,-9,-8,-7,-6,-5,-4,-3,-2,-1,
	0,1,2,4,5,6,7,8,9,10,11,12,12,13,13,13,
	13,13,13,13,12,12,11,10,9,8,7,6,5,4,2,1,
	0,-1,-2,-4,-5,-6,-7,-8,-9,-10,-11,-12,-12,-13,-13,-13,
	-13,-13,-13,-13,-12,-12,-11,-10,-9,-8,-7,-6,-5,-4,-2,-1,
	0,1,3,4,6,7,8,10,11,12,13,14,14,15,15,15,
	15,15,15,15,14,14,13,12,11,10,8,7,6,4,3,1,
	0,-1,-3,-4,-6,-7,-8,-10,-11,-12,-13,-14,-14,-15,-15,-15,
	-15,-15,-15,-15,-14,-14,-13,-12,-11,-10,-8,-7,-6,-4,-3,-1,
	0,1,3,5,6,8,9,11,12,13,14,15,16,17,17,17,
	17,17,17,17,16,15,14,13,12,11,9,8,6,5,3,1,
	0,-1,-3,-5,-6,-8,-9,-11,-12,-13,-14,-15,-16,-17,-17,-17,
	-17,-17,-17,-17,-16,-15,-14,-13,-12,-11,-9,-8,-6,-5,-3,-1,
	0,1,3,5,7,9,11,12,14,15,16,17,18,19,19,19,
	19,19,19,19,18,17,16,15,14,12,11,9,7,5,3,1,
	0,-1,-3,-5,-7,-9,-11,-12,-14,-15,-16,-17,-18,-19,-19,-19,
	-19,-19,-19,-19,-18,-17,-16,-15,-14,-12,-11,-9,-7,-5,-3,-1,
	0,2,4,6,8,10,12,13,15,16,18,19,20,20,21,21,
	21,21,21,20,20,19,18,16,15,13,12,10,8,6,4,2,
	0,-2,-4,-6,-8,-10,-12,-13,-15,-16,-18,-19,-20,-20,-21,-21,
	-21,-21,-21,-20,-20,-19,-18,-16,-15,-13,-12,-10,-8,-6,-4,-2,
	0,2,4,6,9,11,13,15,16,18,19,21,22,22,23,23,
	23,23,23,22,22,21,19,18,16,15,13,11,9,6,4,2,
	0,-2,-4,-6,-9,-11,-13,-15,-16,-18,-19,-21,-22,-22,-23,-23,
	-23,-23,-23,-22,-22,-21,-19,-18,-16,-15,-13,-11,-9,-6,-4,-2,
	0,2,4,7,9,12,14,16,18,20,21,22,23,24,25,25,
	25,25,25,24,23,22,21,20,18,16,14,12,9,7,4,2,
	0,-2,-4,-7,-9,-12,-14,-16,-18,-20,-21,-22,-23,-24,-25,-25,
	-25,-25,-25,-24,-23,-22,-21,-20,-18,-16,-14,-12,-9,-7,-4,-2,
	0,2,5,8,10,13,15,17,19,21,23,24,25,26,27,27,
	27,27,27,26,25,24,23,21,19,17,15,13,10,8,5,2,
	0,-2,-5,-8,-10,-13,-15,-17,-19,-21,-23,-24,-25,-26,-27,-27,
	-27,-27,-27,-26,-25,-24,-23,-21,-19,-17,-15,-13,-10,-8,-5,-2,
	0,2,5,8,11,14,16,18,21,23,24,26,27,28,29,29,
	29,29,29,28,27,26,24,23,21,18,16,14,11,8,5,2,
	0,-2,-5,-8,-11,-14,-16,-18,-21,-23,-24,-26,-27,-28,-29,-29,
	-29,-29,-29,-28,-27,-26,-24,-23,-21,-18,-16,-14,-11,-8,-5,-2
};

static const BYTE mt_VibratoSawTable[] = {
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,1,1,1,1,1,1,1,1,
	2,2,2,2,2,2,2,2,3,3,3,3,3,3,3,3,
	-3,-3,-3,-3,-3,-3,-3,-3,-2,-2,-2,-2,-2,-2,-2,-2,
	-1,-1,-1,-1,-1,-1,-1,-1,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,1,1,1,1,1,2,2,2,2,2,
	3,3,3,3,3,3,4,4,4,4,4,5,5,5,5,5,
	-5,-5,-5,-5,-5,-5,-4,-4,-4,-4,-4,-3,-3,-3,-3,-3,
	-2,-2,-2,-2,-2,-2,-1,-1,-1,-1,-1,0,0,0,0,0,
	0,0,0,0,1,1,1,1,2,2,2,2,3,3,3,3,
	4,4,4,4,5,5,5,5,6,6,6,6,7,7,7,7,
	-7,-7,-7,-7,-6,-6,-6,-6,-5,-5,-5,-5,-4,-4,-4,-4,
	-3,-3,-3,-3,-2,-2,-2,-2,-1,-1,-1,-1,0,0,0,0,
	0,0,0,0,1,1,1,2,2,2,3,3,3,4,4,4,
	5,5,5,5,6,6,6,7,7,7,8,8,8,9,9,9,
	-9,-9,-9,-9,-8,-8,-8,-7,-7,-7,-6,-6,-6,-5,-5,-5,
	-4,-4,-4,-4,-3,-3,-3,-2,-2,-2,-1,-1,-1,0,0,0,
	0,0,0,1,1,1,2,2,3,3,3,4,4,4,5,5,
	6,6,6,7,7,7,8,8,9,9,9,10,10,10,11,11,
	-11,-11,-11,-10,-10,-10,-9,-9,-8,-8,-8,-7,-7,-7,-6,-6,
	-5,-5,-5,-4,-4,-4,-3,-3,-2,-2,-2,-1,-1,-1,0,0,
	0,0,0,1,1,2,2,3,3,3,4,4,5,5,6,6,
	7,7,7,8,8,9,9,10,10,10,11,11,12,12,13,13,
	-13,-13,-13,-12,-12,-11,-11,-10,-10,-10,-9,-9,-8,-8,-7,-7,
	-6,-6,-6,-5,-5,-4,-4,-3,-3,-3,-2,-2,-1,-1,0,0,
	0,0,1,1,2,2,3,3,4,4,5,5,6,6,7,7,
	8,8,9,9,10,10,11,11,12,12,13,13,14,14,15,15,
	-15,-15,-14,-14,-13,-13,-12,-12,-11,-11,-10,-10,-9,-9,-8,-8,
	-7,-7,-6,-6,-5,-5,-4,-4,-3,-3,-2,-2,-1,-1,0,0,
	0,0,1,1,2,2,3,3,4,5,5,6,6,7,7,8,
	9,9,10,10,11,11,12,12,13,14,14,15,15,16,16,17,
	-17,-17,-16,-16,-15,-15,-14,-13,-13,-12,-12,-11,-11,-10,-10,-9,
	-8,-8,-7,-7,-6,-6,-5,-4,-4,-3,-3,-2,-2,-1,-1,0,
	0,0,1,1,2,3,3,4,5,5,6,6,7,8,8,9,
	10,10,11,11,12,13,13,14,15,15,16,16,17,18,18,19,
	-19,-19,-18,-18,-17,-16,-16,-15,-14,-14,-13,-13,-12,-11,-11,-10,
	-9,-9,-8,-8,-7,-6,-6,-5,-4,-4,-3,-3,-2,-1,-1,0,
	0,0,1,2,2,3,4,4,5,6,6,7,8,8,9,10,
	11,11,12,13,13,14,15,15,16,17,17,18,19,19,20,21,
	-21,-21,-20,-19,-19,-18,-17,-17,-16,-15,-15,-14,-13,-12,-12,-11,
	-10,-10,-9,-8,-8,-7,-6,-6,-5,-4,-4,-3,-2,-1,-1,0,
	0,0,1,2,3,3,4,5,6,6,7,8,9,9,10,11,
	12,12,13,14,15,15,16,17,18,18,19,20,21,21,22,23,
	-23,-23,-22,-21,-20,-20,-19,-18,-17,-17,-16,-15,-14,-14,-13,-12,
	-11,-11,-10,-9,-8,-8,-7,-6,-5,-5,-4,-3,-2,-2,-1,0,
	0,0,1,2,3,4,4,5,6,7,8,8,9,10,11,12,
	13,13,14,15,16,17,17,18,19,20,21,21,22,23,24,25,
	-25,-25,-24,-23,-22,-21,-21,-20,-19,-18,-17,-16,-16,-15,-14,-13,
	-12,-12,-11,-10,-9,-8,-8,-7,-6,-5,-4,-3,-3,-2,-1,0,
	0,0,1,2,3,4,5,6,7,7,8,9,10,11,12,13,
	14,14,15,16,17,18,19,20,21,21,22,23,24,25,26,27,
	-27,-27,-26,-25,-24,-23,-22,-21,-20,-20,-19,-18,-17,-16,-15,-14,
	-13,-13,-12,-11,-10,-9,-8,-7,-6,-6,-5,-4,-3,-2,-1,0,
	0,0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,
	15,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,
	-29,-28,-28,-27,-26,-25,-24,-23,-22,-21,-20,-19,-18,-17,-16,-15,
	-14,-13,-13,-12,-11,-10,-9,-8,-7,-6,-5,-4,-3,-2,-1,0
};

static const BYTE mt_VibratoRectTable[] = {
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
	1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,
	3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,
	-3,-3,-3,-3,-3,-3,-3,-3,-3,-3,-3,-3,-3,-3,-3,-3,
	-3,-3,-3,-3,-3,-3,-3,-3,-3,-3,-3,-3,-3,-3,-3,-3,
	5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
	5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
	-5,-5,-5,-5,-5,-5,-5,-5,-5,-5,-5,-5,-5,-5,-5,-5,
	-5,-5,-5,-5,-5,-5,-5,-5,-5,-5,-5,-5,-5,-5,-5,-5,
	7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
	7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
	-7,-7,-7,-7,-7,-7,-7,-7,-7,-7,-7,-7,-7,-7,-7,-7,
	-7,-7,-7,-7,-7,-7,-7,-7,-7,-7,-7,-7,-7,-7,-7,-7,
	9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,
	9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,
	-9,-9,-9,-9,-9,-9,-9,-9,-9,-9,-9,-9,-9,-9,-9,-9,
	-9,-9,-9,-9,-9,-9,-9,-9,-9,-9,-9,-9,-9,-9,-9,-9,
	11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,
	11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,
	-11,-11,-11,-11,-11,-11,-11,-11,-11,-11,-11,-11,-11,-11,-11,-11,
	-11,-11,-11,-11,-11,-11,-11,-11,-11,-11,-11,-11,-11,-11,-11,-11,
	13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,
	13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,
	-13,-13,-13,-13,-13,-13,-13,-13,-13,-13,-13,-13,-13,-13,-13,-13,
	-13,-13,-13,-13,-13,-13,-13,-13,-13,-13,-13,-13,-13,-13,-13,-13,
	15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,
	15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,
	-15,-15,-15,-15,-15,-15,-15,-15,-15,-15,-15,-15,-15,-15,-15,-15,
	-15,-15,-15,-15,-15,-15,-15,-15,-15,-15,-15,-15,-15,-15,-15,-15,
	17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,
	17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,
	-17,-17,-17,-17,-17,-17,-17,-17,-17,-17,-17,-17,-17,-17,-17,-17,
	-17,-17,-17,-17,-17,-17,-17,-17,-17,-17,-17,-17,-17,-17,-17,-17,
	19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
	19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
	-19,-19,-19,-19,-19,-19,-19,-19,-19,-19,-19,-19,-19,-19,-19,-19,
	-19,-19,-19,-19,-19,-19,-19,-19,-19,-19,-19,-19,-19,-19,-19,-19,
	21,21,21,21,21,21,21,21,21,21,21,21,21,21,21,21,
	21,21,21,21,21,21,21,21,21,21,21,21,21,21,21,21,
	-21,-21,-21,-21,-21,-21,-21,-21,-21,-21,-21,-21,-21,-21,-21,-21,
	-21,-21,-21,-21,-21,-21,-21,-21,-21,-21,-21,-21,-21,-21,-21,-21,
	23,23,23,23,23,23,23,23,23,23,23,23,23,23,23,23,
	23,23,23,23,23,23,23,23,23,23,23,23,23,23,23,23,
	-23,-23,-23,-23,-23,-23,-23,-23,-23,-23,-23,-23,-23,-23,-23,-23,
	-23,-23,-23,-23,-23,-23,-23,-23,-23,-23,-23,-23,-23,-23,-23,-23,
	25,25,25,25,25,25,25,25,25,25,25,25,25,25,25,25,
	25,25,25,25,25,25,25,25,25,25,25,25,25,25,25,25,
	-25,-25,-25,-25,-25,-25,-25,-25,-25,-25,-25,-25,-25,-25,-25,-25,
	-25,-25,-25,-25,-25,-25,-25,-25,-25,-25,-25,-25,-25,-25,-25,-25,
	27,27,27,27,27,27,27,27,27,27,27,27,27,27,27,27,
	27,27,27,27,27,27,27,27,27,27,27,27,27,27,27,27,
	-27,-27,-27,-27,-27,-27,-27,-27,-27,-27,-27,-27,-27,-27,-27,-27,
	-27,-27,-27,-27,-27,-27,-27,-27,-27,-27,-27,-27,-27,-27,-27,-27,
	29,29,29,29,29,29,29,29,29,29,29,29,29,29,29,29,
	29,29,29,29,29,29,29,29,29,29,29,29,29,29,29,29,
	-29,-29,-29,-29,-29,-29,-29,-29,-29,-29,-29,-29,-29,-29,-29,-29,
	-29,-29,-29,-29,-29,-29,-29,-29,-29,-29,-29,-29,-29,-29,-29,-29
};

typedef void (*tVoidFn)(void);
typedef void (*tFx)(
	UWORD uwCmd, tChannelStatus *pChannelData,
	volatile struct AudChannel *pChannelReg
);

typedef struct _tModSampleHeader {
	char szName[22];
	UWORD uwLength; ///< Sample data length, in words.
	UBYTE ubNibble; ///< Finetune.
	UBYTE ubVolume;
	UWORD uwRepeatOffs; ///< In words.
	UWORD uwRepeatLength; ///< In words.
} tModSampleHeader;

typedef struct _tModFileHeader {
	char szSongName[20];
	tModSampleHeader pSamples[31];
	UBYTE ubArrangementLength; ///< Length of arrangement, not to be confused with
	                           /// pattern count in file. Max 128.
	UBYTE ubSongEndPos;
	UBYTE pArrangement[128]; ///< song arrangmenet list (pattern Table).
		                       /// These list up to 128 pattern numbers
													 /// and the order they should be played in.
	char pFileFormatTag[4];
	// MOD pattern/sample data follows
} tModFileHeader;

/**
 * Each pattern line consists of following data for each channel.
 */
typedef struct _tModVoice {
	UWORD uwNote;
	UWORD uwCmd;
} tModVoice;

typedef struct _tChannelStatus {
	UWORD n_note;
	UBYTE n_cmd;
	UBYTE n_cmdlo;
	ULONG n_start;
	UWORD *n_loopstart;
	UWORD n_length;
	UWORD n_replen;
	UWORD n_period;
	UWORD n_volume;
	UWORD *n_pertab; ///< Period table, used in arpeggio
	UWORD n_dmabit;
	UWORD n_noteoff;
	UWORD n_toneportspeed;
	UWORD n_wantedperiod;
	UWORD n_pattpos;
	UWORD n_funk;
	ULONG n_wavestart;
	UWORD n_reallength;
	UWORD n_intbit;
	UWORD n_sfxlen;
	UWORD *n_sfxptr;
	UWORD n_sfxper;
	UWORD n_sfxvol;
	UBYTE n_looped;
	UBYTE n_minusft;
	UBYTE n_vibratoamp;
	UBYTE n_vibratospd;
	UBYTE n_vibratopos;
	UBYTE n_vibratoctrl;
	UBYTE n_tremoloamp;
	UBYTE n_tremolospd;
	UBYTE n_tremolopos;
	UBYTE n_tremoloctrl;
	UBYTE n_gliss;
	UBYTE n_sampleoffset;
	UBYTE n_loopcount;
	UBYTE n_funkoffset;
	UBYTE n_retrigcount;
	UBYTE n_sfxpri;
	UBYTE n_freecnt;
	UBYTE n_musiconly;
} tChannelStatus;

static const tFx fx_tab[16];
static const tFx blmorefx_tab[16];

UBYTE mt_MusicChannels = 0;
UBYTE mt_E8Trigger = 0;
UBYTE mt_Enable = 0;

tChannelStatus mt_chan[4];
UWORD *mt_SampleStarts[31];
tModFileHeader *mt_mod;
tVoidFn mt_oldLev6;
ULONG mt_timerval;
UBYTE mt_oldtimers[4];
UBYTE *mt_MasterVolTab;
UWORD mt_Lev6Ena;
UWORD mt_PatternPos;
UWORD mt_PBreakPos; ///< Pattern break pos
UBYTE mt_PosJumpFlag;
UBYTE mt_PBreakFlag;
UBYTE mt_Speed;
UBYTE mt_Counter;
UBYTE mt_SongPos; ///< Position in arrangement.
UBYTE mt_PattDelTime;
UBYTE mt_PattDelTime2;
UBYTE mt_SilCntValid;
UWORD mt_dmaon = DMAF_SETCLR;

tVoidFn *mt_Lev6Int;

static void ptSongStep(void) {
	mt_PatternPos = mt_PBreakPos;
	mt_PBreakPos = 0;
	mt_PosJumpFlag = 0;

	// Next position in song
	UBYTE ubNextPos = (mt_SongPos + 1) & 0x7F;
	// End of song reached?
	if(ubNextPos >= mt_mod->ubArrangementLength) {
		ubNextPos = 0;
	}
	mt_SongPos = ubNextPos;
	// Should be a check of mt_PosJumpFlag here, but unlikely that something will
	// set it in the meantime
}

static void startSfx(
	UWORD uwLen, tChannelStatus *pChannelData,
	volatile struct AudChannel *pChannelReg
) {
	// play new sound effect on this channel
	g_pCustom->dmacon = pChannelData->n_dmabit;
	pChannelReg->ac_ptr = pChannelData->n_sfxptr;
	pChannelReg->ac_len = uwLen;
	pChannelReg->ac_per = pChannelData->n_sfxper;
	pChannelReg->ac_vol = pChannelData->n_sfxvol;

	// save repeat and period for TimerB interrupt
	pChannelData->n_loopstart = pChannelData->n_sfxptr;
	pChannelData->n_replen = 1;
	pChannelData->n_period = pChannelData->n_period;
	pChannelData->n_looped = 0;
	pChannelData->n_sfxlen = 0;

	mt_dmaon |= pChannelData->n_dmabit;
}

void moreBlockedFx(
	UWORD uwCmd, tChannelStatus *pChannelData,
	volatile struct AudChannel *pChannelReg
) {
	blmorefx_tab[uwCmd >> 8](uwCmd, pChannelData, pChannelReg);
}

static void mt_playvoice(
	tChannelStatus *pChannelData, volatile struct AudChannel *pChannelReg,
	tModVoice *pVoice
) {
	// channel blocked by external sound effect?
	if(pChannelData->n_sfxpri) {
		if(pChannelData->n_sfxlen) {
			startSfx(pChannelData->n_sfxlen, pChannelData, pChannelReg);
			moreBlockedFx(pVoice->uwCmd, pChannelData, pChannelReg);
			return;
		}
		// do only some limited commands, while sound effect is in progress
		if(
			!(pChannelData->n_intbit & g_pCustom->intreqr) ||
			(pChannelData->n_dmabit & mt_dmaon)
		) {
			moreBlockedFx(pVoice->uwCmd, pChannelData, pChannelReg);
			return;
		}
		// sound effect sample has played, so unblock this channel again
		pChannelData->n_sfxpri = 0;
	}
	// n_note/cmd: any note or cmd set?
	if(pChannelData->n_note) {
		pChannelReg->ac_per = pChannelData->n_period;
	}
	pChannelData->n_note = pVoice->uwNote;
	pChannelData->n_cmd = pVoice->uwCmd >> 8;
	pChannelData->n_cmdlo = pVoice->uwCmd & 0xFF;

	UWORD d5 = (pChannelData->n_cmd & 0x000F) * 2; // cmd * 2
	// cmd argument in MSW
	ULONG d4 = (pChannelData->n_cmd & 0x00FF) << 16;
	// for checking E-cmd in LSW
	d4 |= pChannelData->n_cmd & 0x0FF0;

	// TODO: implement rest
}

static void mt_checkfx(
	tChannelStatus *pChannelData, volatile struct AudChannel *pChannelReg
) {
	if(pChannelData->n_sfxpri) {
		UWORD uwLen = pChannelData->n_sfxlen;
		if(uwLen) {
			startSfx(uwLen, pChannelData, pChannelReg);
		}
		if(uwLen || (
			(pChannelData->n_intbit & g_pCustom->intreqr) ||
			(pChannelData->n_dmabit & g_pCustom->dmaconr)
		)) {
			// Channel is blocked, only check some E-commands
			UWORD uwCmd = pChannelData->n_cmd & 0x0FFF;
			if((uwCmd & 0xF00) == 0xE00) {
				// NOP command
				return;
			}
			goto lBlockedECmds;
		}
		else {
			// sound effect sample has played, so unblock this channel again
			pChannelData->n_sfxpri = 0;
		}
	}
	// do channel effects between notes
	if(pChannelData->n_funk) {
		mt_updatefunk();
	}
	UWORD uwCmd = pChannelData->n_cmd & 0x0FFF;
	if(!uwCmd) {
		// Just set the current period
		pChannelReg->ac_per = pChannelData->n_period;
	}
	else {
		uwCmd &= 0xFF;
		UWORD uwCmd2 = (pChannelData->n_cmd & 0xF);
		fx_tab[uwCmd2]();
	}
}

static void mt_sfxonly(void);

// TimerA interrupt calls _mt_music at a selectable tempo (Fxx command),
// which defaults to 50 times per second.
static void mt_TimerAInt(void) {
	// clear EXTER interrupt flag
	g_pCustom->intreq = INTF_EXTER;

	// check and clear CIAB interrupt flags
	if(g_pCiaB->icr & CIAICR_TIMER_A) {
		// it was a TA interrupt, do music when enabled
		if(mt_Enable) {
			// music with sfx inserted
			mt_music();
		}
		else {
			// no music, only sfx
			mt_sfxonly();
		}
	}
}

// One-shot TimerB interrupt to set repeat samples after another 496 ticks.
static void mt_TimerBsetrep(void) {
	// check and clear CIAB interrupt flags
	if(g_pCiaB->icr & CIAICR_TIMER_B) {

		// clear EXTER and possible audio interrupt flags
		// KaiN's note: Audio DMAs are 0..3 whereas INTs are (0..3) << 7
		g_pCustom->intreq = INTF_EXTER | (mt_dmaon & 0xFF) << 7;

		// it was a TB interrupt, set repeat sample pointers and lengths
		g_pCustom->aud[0].ac_ptr = mt_chan[0].n_loopstart;
		g_pCustom->aud[0].ac_len = mt_chan[0].n_replen;
		g_pCustom->aud[1].ac_ptr = mt_chan[1].n_loopstart;
		g_pCustom->aud[1].ac_len = mt_chan[1].n_replen;
		g_pCustom->aud[2].ac_ptr = mt_chan[2].n_loopstart;
		g_pCustom->aud[2].ac_len = mt_chan[2].n_replen;
		g_pCustom->aud[3].ac_ptr = mt_chan[3].n_loopstart;
		g_pCustom->aud[3].ac_len = mt_chan[3].n_replen;


		// restore TimerA music interrupt vector
		*mt_Lev6Int = mt_TimerAInt;
	}

	// just clear EXTER interrupt flag and return
	g_pCustom->intreq = INTF_EXTER;
}

// One-shot TimerB interrupt to enable audio DMA after 496 ticks.
static void mt_TimerBdmaon(void) {
	// clear EXTER interrupt flag
	g_pCustom->intreq = INTF_EXTER;

	// check and clear CIAB interrupt flags
	if(g_pCiaB->icr & CIAICR_TIMER_B) {
		// it was a TB interrupt, restart timer to set repeat, enable DMA
		g_pCiaB->crb = 0x19;
		g_pCustom->dmacon = mt_dmaon;

		// set level 6 interrupt to mt_TimerBsetrep
		*mt_Lev6Int = mt_TimerBsetrep;
	}
}

static void chan_sfx_only(
	volatile struct AudChannel *pChannelReg, tChannelStatus *pChannelData
) {
	if(pChannelData->n_sfxpri <= 0) {
		return;
	}
	startSfx(pChannelData->n_sfxlen, pChannelData, pChannelReg);

	if(
		(pChannelData->n_intbit & g_pCustom->intreqr) &&
		(pChannelData->n_dmabit & mt_dmaon)
	) {
		// Last sound effect sample has played, so unblock this channel again
		pChannelData->n_sfxpri = 0;
	}
}

// Called from interrupt.
// Plays sound effects on free channels.
void mt_sfxonly(void) {
	mt_dmaon &= 0xFF00;
	chan_sfx_only(&g_pCustom->aud[0], &mt_chan[0]);
	chan_sfx_only(&g_pCustom->aud[1], &mt_chan[1]);
	chan_sfx_only(&g_pCustom->aud[2], &mt_chan[2]);
	chan_sfx_only(&g_pCustom->aud[3], &mt_chan[3]);

	if(mt_dmaon & 0xFF) {
		*mt_Lev6Int = mt_TimerBdmaon;
		g_pCiaB->crb = 0x19; // load/start timer B, one-shot
	}
}

// The replayer routine. Is called automatically after mt_install_cia().
// Called from interrupt.
// Play next position when Counter equals Speed.
// Effects are always handled.
void mt_music(void) {
	mt_dmaon &= 0xFF00;
	++mt_Counter;
	if(mt_Counter < mt_Speed) {
		// no new note, just check effects, don't step to next position
		mt_checkfx(&mt_chan[0], &g_pCustom->aud[0]);
		mt_checkfx(&mt_chan[1], &g_pCustom->aud[1]);
		mt_checkfx(&mt_chan[2], &g_pCustom->aud[2]);
		mt_checkfx(&mt_chan[3], &g_pCustom->aud[3]);

		// set one-shot TimerB interrupt for enabling DMA, when needed
		if(mt_dmaon & 0xFF) {
			*mt_Lev6Int = mt_TimerBdmaon;
			g_pCiaB->crb = 0x19; // load/start timer B, one-shot
		}
	}
	else {
		// handle a new note
		mt_Counter = 0;
		if(mt_PattDelTime2 <= 0) {
			// determine pointer to current pattern line
			tModSampleHeader *pSamples = mt_mod->pSamples;
			UBYTE *pPatternData = &((UBYTE*)mt_mod)[sizeof(tModFileHeader)];
			UBYTE *pArrangement = mt_mod->pArrangement;
			UBYTE ubPatternIdx = pArrangement[mt_SongPos];
			UBYTE *pCurrentPattern = &pPatternData[ubPatternIdx * 1024];
			tModVoice *pLineVoices = (tModVoice*)&pCurrentPattern[mt_PatternPos];

			// play new note for each channel, apply some effects
			mt_playvoice(&mt_chan[0], &g_pCustom->aud[0], &pLineVoices[0]);
			mt_playvoice(&mt_chan[1], &g_pCustom->aud[1], &pLineVoices[1]);
			mt_playvoice(&mt_chan[2], &g_pCustom->aud[2], &pLineVoices[2]);
			mt_playvoice(&mt_chan[3], &g_pCustom->aud[3], &pLineVoices[3]);
		}
		else {
			// we have a pattern delay, check effects then step
			mt_checkfx(&mt_chan[0], &g_pCustom->aud[0]);
			mt_checkfx(&mt_chan[1], &g_pCustom->aud[1]);
			mt_checkfx(&mt_chan[2], &g_pCustom->aud[2]);
			mt_checkfx(&mt_chan[3], &g_pCustom->aud[3]);
		}
		// set one-shot TimerB interrupt for enabling DMA, when needed
		if(mt_dmaon & 0xFF) {
			*mt_Lev6Int = mt_TimerBdmaon;
			g_pCiaB->crb = 0x19; // load/start timer B, one-shot
		}

		// next pattern line, handle delay and break
		mt_SilCntValid = 0; // recalculate silence counters
		UBYTE ubOffs = 16; // Offset to next pattern line
		UBYTE d1 = mt_PattDelTime2;
		if(mt_PattDelTime) {
			d1 = mt_PattDelTime;
			mt_PattDelTime = 0;
		}
		if(d1) {
			--d1;
			if(d1) {
				ubOffs = 0; // Do not advance to next line
			}
			mt_PattDelTime2 = d1;
		}
		UWORD uwNextLinePos = mt_PatternPos + ubOffs;

		// Check for break
		if(mt_PBreakFlag) {
			mt_PBreakFlag = 0;
			uwNextLinePos = mt_PBreakPos;
			mt_PBreakPos = 0;
		}

		// Check whether end of pattern is reached
		mt_PatternPos = uwNextLinePos;
		if(uwNextLinePos >= 1024) {
			ptSongStep();
		}
	}
	if(mt_PosJumpFlag) {
		ptSongStep();
	}
}

// Stop playing current module.
void mt_end(void) {
	g_pCustom->aud[0].ac_vol = 0;
	g_pCustom->aud[1].ac_vol = 0;
	g_pCustom->aud[2].ac_vol = 0;
	g_pCustom->aud[3].ac_vol = 0;
	g_pCustom->dmacon = DMAF_AUDIO;
}

void mt_reset(void) {
	// reset speed and counters
	mt_Speed = 6;
	mt_Counter = 0;
	mt_PatternPos = 0;

	// Disable the filter
	g_pCiaA->pra |= 0x02;

	// set master volume to 64
	mt_MasterVolTab = MasterVolTab[64];

	// initialise channel DMA, interrupt bits and audio register base
	mt_chan[0].n_dmabit = DMAF_AUD0;
	mt_chan[1].n_dmabit = DMAF_AUD1;
	mt_chan[2].n_dmabit = DMAF_AUD2;
	mt_chan[3].n_dmabit = DMAF_AUD3;
	mt_chan[0].n_intbit = INTF_AUD0;
	mt_chan[1].n_intbit = INTF_AUD1;
	mt_chan[2].n_intbit = INTF_AUD2;
	mt_chan[3].n_intbit = INTF_AUD3;

	// make sure n_period doesn't start as 0
	mt_chan[0].n_period = 320;
	mt_chan[1].n_period = 320;
	mt_chan[2].n_period = 320;
	mt_chan[3].n_period = 320;

	// disable sound effects
	mt_chan[0].n_sfxlen = 0;
	mt_chan[1].n_sfxlen = 0;
	mt_chan[2].n_sfxlen = 0;
	mt_chan[3].n_sfxlen = 0;

	mt_SilCntValid = 0;
	mt_E8Trigger = 0;
	mt_end();
}

void mt_install_cia(APTR *AutoVecBase, UBYTE PALflag) {
	mt_Enable = 0;

	// Level 6 interrupt vector
	mt_Lev6Int = (void*)&AutoVecBase[0x78/sizeof(ULONG)];

	// remember level 6 interrupt enable
	mt_Lev6Ena = (g_pCustom->intenar & INTF_EXTER) | INTF_SETCLR;

	// disable level 6 EXTER interrupts, set player interrupt vector
	g_pCustom->intena = INTF_EXTER;
	mt_oldLev6 = *mt_Lev6Int;
	*mt_Lev6Int = mt_TimerAInt;

	// disable CIA-B interrupts, stop and save all timers
	g_pCiaB->icr = 0x7f;
	g_pCiaB->cra = 0x10;
	g_pCiaB->crb = 0x10;
	mt_oldtimers[0] = g_pCiaB->talo;
	mt_oldtimers[1] = g_pCiaB->tahi;
	mt_oldtimers[2] = g_pCiaB->tblo;
	mt_oldtimers[3] = g_pCiaB->tbhi;

	// determine if 02 clock for timers is based on PAL or NTSC
	if(PALflag) {
		mt_timerval = 1773447;
	}
	else {
		mt_timerval = 1789773;
	}

	// load TimerA in continuous mode for the default tempo of 125
	g_pCiaB->talo = 125;
	g_pCiaB->tahi = 8;
	g_pCiaB->cra = 0x11; // load timer, start continuous

	// load TimerB with 496 ticks for setting DMA and repeat
	g_pCiaB->tblo = 496 & 0xFF;
	g_pCiaB->tbhi = 496 >> 8;

	// TimerA and TimerB interrupt enable
	g_pCiaB->icr = 0x83;

	// enable level 6 interrupts
	g_pCustom->intena = INTF_SETCLR | INTF_EXTER;

	mt_reset();
}

void mt_remove_cia(void) {
	// disable level 6 and CIA-B interrupts
	g_pCiaB->icr = 0x7F;
	g_pCustom->intena = INTF_EXTER;

	// restore old timer values
	g_pCiaB->talo = mt_oldtimers[0];
	g_pCiaB->tahi = mt_oldtimers[1];
	g_pCiaB->tblo = mt_oldtimers[2];
	g_pCiaB->tbhi = mt_oldtimers[3];
	g_pCiaB->cra = 0x10;
	g_pCiaB->crb = 0x10;

	// restore original level 6 interrupt vector
	*mt_Lev6Int = mt_oldLev6;

	// reenable CIA-B ALRM interrupt, which was set by AmigaOS
	g_pCiaB->icr = 0x84;

	// reenable previous level 6 interrupt
	g_pCustom->intena = mt_Lev6Ena;
}

// Patterns - each has 64 rows, each row has 4 notes, each note has 4 bytes
// Length of single pattern.
#define MOD_PATTERN_LENGTH 1024

void mt_init(UBYTE *TrackerModule, UBYTE *Samples, UWORD InitialSongPos) {
	// Initialize new module.
	// Reset speed to 6, tempo to 125 and start at given song position.
	// Master volume is at 64 (maximum).

	mt_mod = (tModFileHeader*)TrackerModule;

	// set initial song position
	if(InitialSongPos >= 950) {
		InitialSongPos = 0;
	}
	mt_SongPos = InitialSongPos;

	// sample data location is given?
	if(!Samples) {
		// Get number of highest pattern
		UBYTE ubLastPattern = 0;
		for(UBYTE i = 0; i < 127; ++i) {
			if(mt_mod->pArrangement[i] > ubLastPattern) {
				ubLastPattern = mt_mod->pArrangement[i];
			}
		}
		UBYTE ubPatternCount = ubLastPattern + 1;

		// now we can calculate the base address of the sample data
		ULONG ulSampleOffs = (
			sizeof(tModFileHeader) + ubPatternCount * MOD_PATTERN_LENGTH
		);
		Samples = &TrackerModule[ulSampleOffs];
		// FIXME: use as pointer for empty samples
	}

	tModSampleHeader *pHeaders = (tModSampleHeader*)&TrackerModule[42];

	// save start address of each sample
	UBYTE *pSampleCurr = Samples;
	for(UBYTE i = 0; i < 31; ++i) {
		mt_SampleStarts[i] = (UWORD*)&pSampleCurr;

		// make sure sample starts with two 0-bytes
		*mt_SampleStarts[i] = 0;

		// go to next sample
		pSampleCurr += pHeaders[i].uwLength * 2;
	};

	// reset CIA timer A to default (125)
	UWORD uwTimer = mt_timerval / 125;
	g_pCiaB->talo = uwTimer;
	g_pCiaB->tahi = uwTimer >> 8;

	mt_reset();
}

// activate the sound effect on this channel
static void ptSetSfx(tSfxStructure *pSfx, tChannelStatus *pChannel) {
	pChannel->n_sfxptr = pSfx->sfx_ptr;
	pChannel->n_sfxlen = pSfx->sfx_len;
	pChannel->n_sfxper = pSfx->sfx_per;
	pChannel->n_sfxvol = pSfx->sfx_vol;
	pChannel->n_sfxpri = pSfx->sfx_pri;
}

// Request playing of a prioritized external sound effect, either on a
// fixed channel or on the most unused one.
// A negative channel specification means to use the best one.
// The priority is unsigned and should be greater than zero.
// When multiple samples are assigned to the same channel the lower
// priority sample will be replaced. When priorities are the same, then
// the older sample is replaced.
// The chosen channel is blocked for music until the effect has
// completely been replayed.
void mt_playfx(tSfxStructure *SfxStructurePointer) {
	if(SfxStructurePointer->sfx_cha > 0) {
		// use fixed channel for effect
		tChannelStatus *pChannels[] = {&mt_chan[0], &mt_chan[1], &mt_chan[2], &mt_chan[3]};
		tChannelStatus *pChannel = pChannels[SfxStructurePointer->sfx_cha];

		// Priority high enough to replace a present effect on this channel?
		g_pCustom->intena = INTF_INTEN;
		if(SfxStructurePointer->sfx_pri >= pChannel->n_sfxpri) {
			ptSetSfx(SfxStructurePointer, pChannel);
		}
		return;
	}
	// Did we already calculate the n_freecnt values for all channels?
	if(!mt_SilCntValid) {
		// Look at the next 8 pattern steps to find the longest sequence
		// of silence (no new note or instrument).
		UBYTE i = 8;

		// remember which channels are not available for sound effects
		UBYTE pMusicOnly[4];
		for(UBYTE ubChannel = 0; ubChannel < 4; ++ubChannel) {
			pMusicOnly[ubChannel] = mt_chan[ubChannel].n_musiconly;
		}

		// reset freecnts for all channels
		mt_chan[0].n_freecnt = 0;
		mt_chan[1].n_freecnt = 0;
		mt_chan[2].n_freecnt = 0;
		mt_chan[3].n_freecnt = 0;

		// get pattern pointer
		UBYTE *pPatterns = (UBYTE*)mt_mod + sizeof(tModFileHeader);

		UBYTE ubSongPos = mt_SongPos;
		UWORD uwPatternPos = mt_PatternPos;
		UBYTE isEnd = 0;
		UBYTE *pPatternStart = &pPatterns[
			mt_mod->pArrangement[ubSongPos] * MOD_PATTERN_LENGTH
		];
		tModVoice *pPatternEnd = (tModVoice*)(pPatternStart + MOD_PATTERN_LENGTH);
		tModVoice *pPatternPos = (tModVoice*)(pPatternStart + uwPatternPos);
		do {
			UBYTE d0 = 4;

			for(UBYTE ubChannel = 0; ubChannel < 4; ++ubChannel) {
				if(!pMusicOnly[ubChannel]) {
					++mt_chan[0].n_freecnt;
					if(pPatternPos->uwNote) {
						pMusicOnly[ubChannel] = 1;
					}
				}
				++pPatternPos;
				d0 -= pMusicOnly[ubChannel];
			}

			// break the loop when no channel has any more free pattern steps
			// otherwise break after 8 pattern steps
			isEnd = (d0 != 0 && --i != 0);
			// End of pattern reached? Then load next pattern pointer
			if(!isEnd && pPatternPos >= pPatternEnd) {
				uwPatternPos = 0;
				ubSongPos = (mt_SongPos + 1) & 127;
				if(ubSongPos >= mt_mod->ubArrangementLength) {
					ubSongPos = 0;
				}
				pPatternStart = &pPatterns[
					mt_mod->pArrangement[ubSongPos] * MOD_PATTERN_LENGTH
				];
				pPatternEnd = (tModVoice*)(pPatternStart + MOD_PATTERN_LENGTH);
				pPatternPos = (tModVoice*)pPatternStart;
			}
		} while(!isEnd);
		mt_SilCntValid = 1;
	}

	g_pCustom->intena = INTF_INTEN;
	tChannelStatus *pBestChannel = 0;

	// Determine which channels are already allocated for sound
	// effects and check if the limit was reached. In this case only
	// replace sound effect channels by higher priority.
	BYTE bFreeChannels = 3 - mt_MusicChannels;
	if(mt_chan[0].n_sfxpri) {
		bFreeChannels += 1;
	}
	if(mt_chan[1].n_sfxpri) {
		bFreeChannels += 1;
	}
	if(mt_chan[2].n_sfxpri) {
		bFreeChannels += 1;
	}
	if(mt_chan[3].n_sfxpri) {
		bFreeChannels += 1;
	}
	if(bFreeChannels >= 0) {

		// Exclude channels which have set a repeat loop.
		// Try not to break them!
		UWORD uwChannels = 0;
		if(!mt_chan[0].n_looped) {
			uwChannels |= INTF_AUD0;
		}
		else if(!mt_chan[1].n_looped) {
			uwChannels |= INTF_AUD1;
		}
		else if(!mt_chan[2].n_looped) {
			uwChannels |= INTF_AUD2;
		}
		else if(!mt_chan[3].n_looped) {
			uwChannels |= INTF_AUD3;
		}

		// We will prefer a music channel which had an audio interrupt bit set,
		// because that means the last instrument sample has been played
		// completely, and the channel is now in an idle loop.
		uwChannels &= g_pCustom->intreqr;
		if(!uwChannels) {
			// All channels are busy, then it doesn't matter which one we break...
			uwChannels = INTF_AUD0 | INTF_AUD1 | INTF_AUD2 | INTF_AUD3;
		}

		// First look for the best unused channel
		UBYTE ubBestFreeCnt = 0;
		if(!(uwChannels & INTF_AUD0) && !mt_chan[0].n_sfxpri) {
			if(mt_chan[0].n_freecnt > ubBestFreeCnt) {
				ubBestFreeCnt = mt_chan[0].n_freecnt;
				pBestChannel = &mt_chan[0];
			}
		}
		if(!(uwChannels & INTF_AUD1) && !mt_chan[1].n_sfxpri) {
			if(mt_chan[1].n_freecnt > ubBestFreeCnt) {
				ubBestFreeCnt = mt_chan[1].n_freecnt;
				pBestChannel = &mt_chan[1];
			}
		}
		if(!(uwChannels & INTF_AUD2) && !mt_chan[2].n_sfxpri) {
			if(mt_chan[2].n_freecnt > ubBestFreeCnt) {
				ubBestFreeCnt = mt_chan[2].n_freecnt;
				pBestChannel = &mt_chan[2];
			}
		}
		if(!(uwChannels & INTF_AUD3) && !mt_chan[3].n_sfxpri) {
			if(mt_chan[3].n_freecnt > ubBestFreeCnt) {
				ubBestFreeCnt = mt_chan[3].n_freecnt;
				pBestChannel = &mt_chan[3];
			}
		}
	}
	else {
		// Finally try to overwrite a sound effect with lower/equal priority
		UBYTE ubBestFreeCnt = 0;
		if(
			mt_chan[0].n_sfxpri > 0 &&
			mt_chan[0].n_sfxpri < SfxStructurePointer->sfx_pri &&
			mt_chan[0].n_freecnt > ubBestFreeCnt
		) {
			ubBestFreeCnt = mt_chan[0].n_freecnt;
			pBestChannel = &mt_chan[0];
		}
		else if(
			mt_chan[1].n_sfxpri > 0 &&
			mt_chan[1].n_sfxpri < SfxStructurePointer->sfx_pri &&
			mt_chan[1].n_freecnt > ubBestFreeCnt
		) {
			ubBestFreeCnt = mt_chan[1].n_freecnt;
			pBestChannel = &mt_chan[1];
		}
		else if(
			mt_chan[2].n_sfxpri > 0 &&
			mt_chan[2].n_sfxpri < SfxStructurePointer->sfx_pri &&
			mt_chan[2].n_freecnt > ubBestFreeCnt
		) {
			ubBestFreeCnt = mt_chan[2].n_freecnt;
			pBestChannel = &mt_chan[2];
		}
		else if(
			mt_chan[3].n_sfxpri > 0 &&
			mt_chan[3].n_sfxpri < SfxStructurePointer->sfx_pri &&
			mt_chan[3].n_freecnt > ubBestFreeCnt
		) {
			ubBestFreeCnt = mt_chan[3].n_freecnt;
			pBestChannel = &mt_chan[3];
		}
	}
	if(!pBestChannel) {
		return;
	}
	ptSetSfx(SfxStructurePointer, pBestChannel);
}

// Request playing of an external sound effect on the most unused channel.
// This function is for compatibility with the old API only!
// You should call mt_playfx instead.
void mt_soundfx(
	APTR SamplePointer, UWORD SampleLength, UWORD SamplePeriod, UWORD SampleVolume
) {
	tSfxStructure sSfx;
	sSfx.sfx_ptr = SamplePointer;
	sSfx.sfx_len = SampleLength;
	sSfx.sfx_per = SamplePeriod;
	sSfx.sfx_vol = SampleVolume;
	// any channel, priority = 1
	sSfx.sfx_cha = -1;
	sSfx.sfx_pri = 1;
	mt_playfx(&sSfx);
}

// Set bits in the mask define which specific channels are reserved
// for music only. Set bit 0 for channel 0, ..., bit 3 for channel 3.
// When calling _mt_soundfx or _mt_playfx with automatic channel selection
// (sfx_cha=-1) then these masked channels will never be picked.
// The mask defaults to 0.
void mt_musicmask(UBYTE ChannelMask) {
	g_pCustom->intena = INTF_INTEN;
	mt_chan[0].n_musiconly = BTST(ChannelMask, 0);
	mt_chan[1].n_musiconly = BTST(ChannelMask, 1);
	mt_chan[2].n_musiconly = BTST(ChannelMask, 2);
	mt_chan[3].n_musiconly = BTST(ChannelMask, 3);

	g_pCustom->intena = INTF_SETCLR | INTF_INTEN;
}

// Set a master volume from 0 to 64 for all music channels.
// Note that the master volume does not affect the volume of external
// sound effects (which is desired).
void mt_mastervol(UWORD MasterVolume) {
	g_pCustom->intena = INTF_INTEN;
	mt_MasterVolTab = MasterVolTab[MasterVolume];
	g_pCustom->intena = INTF_SETCLR | INTF_INTEN;
}

//-------------------------------------------------- COMMANDS WITHOUT CMD PASSED

static void mt_toneporta_nc(
	tChannelStatus *pChannelData, volatile struct AudChannel *pChannelReg
) {
	if(pChannelData->n_wantedperiod) {
		UWORD uwNew;
		if(pChannelData->n_period > pChannelData->n_wantedperiod) {
			// tone porta up
			uwNew = pChannelData->n_period - pChannelData->n_toneportspeed;
			if(uwNew < pChannelData->n_wantedperiod) {
				uwNew = pChannelData->n_wantedperiod;
				pChannelData->n_wantedperiod = 0;
			}
		}
		else {
			// tone porta down
			uwNew = pChannelData->n_period + pChannelData->n_toneportspeed;
			if(uwNew > pChannelData->n_wantedperiod) {
				uwNew = pChannelData->n_wantedperiod;
				pChannelData->n_wantedperiod = 0;
			}
		}
		pChannelData->n_period = uwNew;
		if(pChannelData->n_gliss) {
			// glissando: find nearest note for new period
			// TODO
		}
		pChannelReg->ac_per = pChannelData->n_period;
	}
}

static void mt_vibrato_nc(
	tChannelStatus *pChannelData, volatile struct AudChannel *pChannelReg,
	UBYTE ubAmplitude, UBYTE ubSpeed
) {
	// calculate vibrato table offset
	UBYTE ubOffs = 64 * ubAmplitude + (pChannelData->n_vibratopos & 63);

	// select vibrato waveform
	UBYTE *pTable;
	UBYTE ubCtl = pChannelData->n_vibratoctrl & 3;
	if(!ubCtl) {
		pTable = mt_VibratoSineTable;
	}
	else if(ubCtl == 1) {
		pTable = mt_VibratoSawTable;
	}
	else {
		// ctrl 2 & 3 select a rectangle vibrato
		pTable = mt_VibratoRectTable;
	}

	// add vibrato-offset to period
	pChannelReg->ac_per = pChannelData->n_period + pTable[ubOffs];

	// Increase vibratopos by speed
	pChannelData->n_vibratopos += ubSpeed;
}

//-------------------------------------------------------- EFFECTS WITH CMD WORD

static void mt_nop(
	UWORD uwCmd, tChannelStatus *pChannelData,
	volatile struct AudChannel *pChannelReg
) {

}

static void mt_arpeggio(
	UWORD uwCmd, tChannelStatus *pChannelData,
	volatile struct AudChannel *pChannelReg
) {
	// cmd 0 x y (x = first arpeggio offset, y = second arpeggio offset)
	static const UBYTE pArpTab[] = {
		0, 1, -1, 0, 1, -1, 0, 1, -1, 0, 1, -1, 0, 1, -1, 0,
		1, -1, 0, 1, -1, 0, 1, -1, 0, 1, -1, 0, 1, -1, 0, 1
	};

	// Step 0, just use normal period
	pChannelReg->ac_per = pChannelData->n_period;

	UWORD uwVal;
	if(pArpTab[mt_Counter] >= 0) {
		// Step 1, arpeggio by left nibble
		uwVal = (uwCmd >> 4) & 0xF;
	}
	else {
		// Step 2, arpeggio by right nibble
		uwVal = uwCmd & 0xF;
	}
	// offset current note
	uwVal *= 2;
	uwVal += pChannelData->n_noteoff;
	// TODO: check if bhs is properly translated here
	if(uwVal < 2 * 36) {
		// Set period with arpeggio offset from note table
		pChannelReg->ac_per = pChannelData->n_pertab[uwVal / 2];
		// TODO: noteoff has byte offs from start of array, set it to divided by 2
		// since we're using UWORD pointers, not UBYTE
	}
}

static void mt_portaup(
	UWORD uwCmd, tChannelStatus *pChannelData,
	volatile struct AudChannel *pChannelReg
) {
	// cmd 1 x x (subtract xx from period)
	do_porta_up(uwCmd);
}

static void mt_portadown(
	UWORD uwCmd, tChannelStatus *pChannelData,
	volatile struct AudChannel *pChannelReg
) {
	// cmd 2 x x (add xx to period)
	do_porta_down(uwCmd);
}

static void mt_toneporta(
	UWORD uwCmd, tChannelStatus *pChannelData,
	volatile struct AudChannel *pChannelReg
) {
	// cmd 3 x y (xy = tone portamento speed)
	if(!(uwCmd & 0xFF)) {
		pChannelData->n_toneportspeed = uwCmd;
		pChannelData->n_cmdlo = 0;
	}
	mt_toneporta_nc(pChannelData, pChannelReg);
}

static void mt_vibrato(
	UWORD uwCmd, tChannelStatus *pChannelData,
	volatile struct AudChannel *pChannelReg
) {
	// cmd 4 x y (x = speed, y = amplitude)
	UBYTE ubAmplitude = uwCmd & 0xF;
	if(ubAmplitude) {
		pChannelData->n_vibratoamp = ubAmplitude;
	}
	else {
		ubAmplitude = pChannelData->n_vibratoamp;
	}
	UBYTE ubSpeed = (uwCmd >> 4) & 0xF;
	if(ubSpeed) {
		pChannelData->n_vibratospd = ubSpeed;
	}
	else {
		ubSpeed = pChannelData->n_vibratospd;
	}
	mt_vibrato_nc(pChannelData, pChannelReg, ubAmplitude, ubSpeed);
}

static const tFx fx_tab[16] = {
	mt_arpeggio,
	mt_portaup,
	mt_portadown,
	mt_toneporta,
	mt_vibrato,
	mt_tonevolslide,
	mt_vibrvolslide,
	mt_tremolo,
	mt_nop,
	mt_nop,
	mt_volumeslide,
	mt_nop,
	mt_nop,
	mt_nop,
	mt_e_cmds,
	mt_nop
};

//---------------------------------------------------------------- MORE FX TABLE

static const tFx blmorefx_tab[16] = {
	mt_nop,
	mt_nop,
	mt_nop,
	mt_nop,
	mt_nop,
	mt_nop,
	mt_nop,
	mt_nop,
	mt_nop,
	mt_nop,
	mt_nop,
	mt_posjump, // 0xB
	mt_nop,
	mt_patternbrk, // 0xD
	blocked_e_cmds,
	mt_setspeed, // 0xF
}
