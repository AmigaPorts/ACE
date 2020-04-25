/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

// Info regarding MOD file format:
// - https://www.fileformat.info/format/mod/corion.htm
// - http://lclevy.free.fr/mo3/mod.txt
// - https://www.stef.be/bassoontracker/docs/ProtrackerCommandReference.pdf
// - http://coppershade.org/articles/More!/Topics/Protracker_Tempo
// - http://coppershade.org/articles/More!/Topics/Protracker_File_Format/

#include <ace/utils/ptplayer.h>
#include <ace/managers/log.h>
#include <ace/managers/system.h>
#include <ace/utils/custom.h>
#include <hardware/intbits.h>
#include <hardware/dmabits.h>

// Flags which modify ptplayer functionality
// #define PTPLAYER_USE_VBL // Otherwise use CIA
// #define PTPLAYER_DEFER_INTERRUPTS

// Patterns - each has 64 rows, each row has 4 notes, each note has 4 bytes
// Length of single pattern.
#define MOD_ROWS_IN_PATTERN 64
#define MOD_NOTES_IN_ROW 4
#define MOD_BYTES_PER_NOTE 4
#define MOD_PATTERN_LENGTH (MOD_ROWS_IN_PATTERN * MOD_NOTES_IN_ROW * MOD_BYTES_PER_NOTE)
#define MOD_PERIOD_TABLE_LENGTH 36

/**
 * @brief Period tables for each finetune value.
 * Each period table consists of periods for notes:
 * C-1, C#1, D-1, D#1, E-1, F-1, F#1, G-1, G#1, A-1, A#1, B-1,
 * C-2, C#2, D-2, D#2, E-2, F-2, F#2, G-2, G#2, A-2, A#2, B-2,
 * C-3, C#3, D-3, D#3, E-3, F-3, F#3, G-3, G#3, A-3, A#3, B-3
 */
static const UWORD mt_PeriodTables[][MOD_PERIOD_TABLE_LENGTH] = {
	{
		856, 808, 762, 720, 678, 640, 604, 570, 538, 508, 480, 453,
		428, 404, 381, 360, 339, 320, 302, 285, 269, 254, 240, 226,
		214, 202, 190, 180, 170, 160, 151, 143, 135, 127, 120, 113
	},
	{
		850, 802, 757, 715, 674, 637, 601, 567, 535, 505, 477, 450,
		425, 401, 379, 357, 337, 318, 300, 284, 268, 253, 239, 225,
		213, 201, 189, 179, 169, 159, 150, 142, 134, 126, 119, 113
	},
	{
		844, 796, 752, 709, 670, 632, 597, 563, 532, 502, 474, 447,
		422, 398, 376, 355, 335, 316, 298, 282, 266, 251, 237, 224,
		211, 199, 188, 177, 167, 158, 149, 141, 133, 125, 118, 112
	},
	{
		838, 791, 746, 704, 665, 628, 592, 559, 528, 498, 470, 444,
		419, 395, 373, 352, 332, 314, 296, 280, 264, 249, 235, 222,
		209, 198, 187, 176, 166, 157, 148, 140, 132, 125, 118, 111
	},
	{
		832, 785, 741, 699, 660, 623, 588, 555, 524, 495, 467, 441,
		416, 392, 370, 350, 330, 312, 294, 278, 262, 247, 233, 220,
		208, 196, 185, 175, 165, 156, 147, 139, 131, 124, 117, 110
	},
	{
		826, 779, 736, 694, 655, 619, 584, 551, 520, 491, 463, 437,
		413, 390, 368, 347, 328, 309, 292, 276, 260, 245, 232, 219,
		206, 195, 184, 174, 164, 155, 146, 138, 130, 123, 116, 109
	},
	{
		820, 774, 730, 689, 651, 614, 580, 547, 516, 487, 460, 434,
		410, 387, 365, 345, 325, 307, 290, 274, 258, 244, 230, 217,
		205, 193, 183, 172, 163, 154, 145, 137, 129, 122, 115, 109
	},
	{
		814, 768, 725, 684, 646, 610, 575, 543, 513, 484, 457, 431,
		407, 384, 363, 342, 323, 305, 288, 272, 256, 242, 228, 216,
		204, 192, 181, 171, 161, 152, 144, 136, 128, 121, 114, 108
	},
	{
		907, 856, 808, 762, 720, 678, 640, 604, 570, 538, 508, 480,
		453, 428, 404, 381, 360, 339, 320, 302, 285, 269, 254, 240,
		226, 214, 202, 190, 180, 170, 160, 151, 143, 135, 127, 120
	},
	{
		900, 850, 802, 757, 715, 675, 636, 601, 567, 535, 505, 477,
		450, 425, 401, 379, 357, 337, 318, 300, 284, 268, 253, 238,
		225, 212, 200, 189, 179, 169, 159, 150, 142, 134, 126, 119
	},
	{
		894, 844, 796, 752, 709, 670, 632, 597, 563, 532, 502, 474,
		447, 422, 398, 376, 355, 335, 316, 298, 282, 266, 251, 237,
		223, 211, 199, 188, 177, 167, 158, 149, 141, 133, 125, 118
	},
	{
		887, 838, 791, 746, 704, 665, 628, 592, 559, 528, 498, 470,
		444, 419, 395, 373, 352, 332, 314, 296, 280, 264, 249, 235,
		222, 209, 198, 187, 176, 166, 157, 148, 140, 132, 125, 118
	},
	{
		881, 832, 785, 741, 699, 660, 623, 588, 555, 524, 494, 467,
		441, 416, 392, 370, 350, 330, 312, 294, 278, 262, 247, 233,
		220, 208, 196, 185, 175, 165, 156, 147, 139, 131, 123, 117
	},
	{
		875, 826, 779, 736, 694, 655, 619, 584, 551, 520, 491, 463,
		437, 413, 390, 368, 347, 328, 309, 292, 276, 260, 245, 232,
		219, 206, 195, 184, 174, 164, 155, 146, 138, 130, 123, 116
	},
	{
		868, 820, 774, 730, 689, 651, 614, 580, 547, 516, 487, 460,
		434, 410, 387, 365, 345, 325, 307, 290, 274, 258, 244, 230,
		217, 205, 193, 183, 172, 163, 154, 145, 137, 129, 122, 115
	},
	{
		862, 814, 768, 725, 684, 646, 610, 575, 543, 513, 484, 457,
		431, 407, 384, 363, 342, 323, 305, 288, 272, 256, 242, 228,
		216, 203, 192, 181, 171, 161, 152, 144, 136, 128, 121, 114
	}
};

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

static const UBYTE mt_FunkTable[] = {
	0, 5, 6, 7, 8, 10, 11, 13, 16, 19, 22, 26, 32, 43, 64, 128
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

typedef struct _tModSampleHeader {
	char szName[22];
	UWORD uwLength; ///< Sample data length, in words.
	UBYTE ubFineTune; ///< Finetune. Only lower nibble is used. Values translate to finetune: {0..7, -8..-1}
	UBYTE ubVolume; ///< Sample volume. 0..64
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
	char pFileFormatTag[4];  ///< Should be "M.K." for 31-sample format.
	// MOD pattern/sample data follows
} tModFileHeader;

typedef struct AudChannel tChannelRegs;

/**
 * Each pattern line consists of following data for each channel.
 */
typedef struct _tModVoice {
	UWORD uwNote; // [instrumentHi:4] [period:12]
	union {
		struct {
			UBYTE ubCmdHi; // [instrumentLo:4] [cmdNo:4]
			UBYTE ubCmdLo; // [cmdArg:8] - special effects data
		};
		UWORD uwCmd; // [instrumentLo:4] [cmdNo:4] [cmdArg:8]
	};
} tModVoice;

typedef struct _tChannelStatus {
	UWORD n_note;
	UBYTE n_cmd; ///< high byte of command word ([instrumentLo:4] [cmdNo:4])
	UBYTE n_cmdlo; ///< [cmdArg:8]
	UWORD *n_start; ///< Pointer to start of sample
	UWORD *n_loopstart;
	UWORD n_length;
	UWORD n_replen;
	UWORD n_period;
	UWORD n_volume;
	const UWORD *n_pertab; ///< Period table, used in arpeggio
	UWORD n_dmabit;
	UWORD n_noteoff; ///< TODO Later: convert it from byte offs to word offs (div by 2)
	UWORD n_toneportspeed;
	UWORD n_wantedperiod;
	UWORD n_pattpos;
	UWORD n_funk; ///< Funk speed
	UBYTE *n_wavestart;
	UWORD n_reallength;
	UWORD n_intbit; ///< channel's INTF_AUD# value.
	UWORD n_sfxlen; ///< channel's DMAF_AUD# value.
	UWORD *n_sfxptr;
	UWORD n_sfxper;
	UWORD n_sfxvol;
	UBYTE n_looped;
	UBYTE n_minusft; ///< Set to 1 on negative finetune value
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
	BYTE n_loopcount;
	BYTE n_funkoffset;
	UBYTE n_retrigcount;
	UBYTE n_sfxpri;
	UBYTE n_freecnt;
	UBYTE n_musiconly;
} tChannelStatus;

typedef void (*tVoidFn)(void);
typedef void (*tFx)(
	UBYTE ubArgs, tChannelStatus *pChannelData,
	volatile tChannelRegs *pChannelReg
);
typedef void (*tEFn)(
	UBYTE ubArg, tChannelStatus *pChannelData,
	volatile tChannelRegs *pChannelReg
);

typedef void (*tPreFx)(
	UWORD uwCmd, UWORD uwCmdArg, UWORD uwMaskedCmdE, const tModVoice *pVoice,
	tChannelStatus *pChannelData, volatile tChannelRegs *pChannelReg
);

static const tFx fx_tab[16];
static const tFx morefx_tab[16];
static const tFx blmorefx_tab[16];
static const tEFn ecmd_tab[16];
static const tEFn blecmd_tab[16];
static const tPreFx prefx_tab[16];

#if defined(PTPLAYER_DEFER_INTERRUPTS)
static volatile UBYTE s_isPendingPlay, s_isPendingSetRep, s_isPendingDmaOn;
#endif
static volatile UWORD s_uwAudioInterrupts;


static void blocked_e_cmds(
	UBYTE ubArgs, tChannelStatus *pChannelData,
	volatile tChannelRegs *pChannelReg
);

static void set_finetune(
	UWORD uwCmd, UWORD uwCmdArg, UWORD uwMaskedCmdE, const tModVoice *pVoice,
	tChannelStatus *pChannelData, volatile tChannelRegs *pChannelReg
);

static volatile UBYTE mt_MusicChannels = 0;
static volatile UBYTE mt_E8Trigger = 0;
static volatile UBYTE mt_Enable = 0;

tChannelStatus mt_chan[4];
UWORD *mt_SampleStarts[31]; ///< Start address of each sample
tModFileHeader *mt_mod;
ULONG mt_timerval; ///< Base nterrupt frequency of CIA-B timer A used to advance the song. Equals 125*50Hz.
UBYTE *mt_MasterVolTab;
UWORD mt_PatternPos;
UWORD mt_PBreakPos; ///< Pattern break pos
UBYTE mt_PosJumpFlag;
UBYTE mt_PBreakFlag;
UBYTE mt_Speed; ///< Number of times the main loop gets called per note step.
UBYTE mt_Counter; ///< Number of main loop iterations in current note step so far.
UBYTE mt_SongPos; ///< Position in arrangement.
UBYTE mt_PattDelTime;
UBYTE mt_PattDelTime2;
UBYTE mt_SilCntValid;
UWORD mt_dmaon = 0; ///< Set by DMAF_AUD# when

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
	volatile tChannelRegs *pChannelReg
) {
	// logWrite("startsfx: %p:%hu\n", pChannelData->n_sfxptr, uwLen);
	// play new sound effect on this channel
	systemSetDmaMask(pChannelData->n_dmabit, 0);
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
	volatile tChannelRegs *pChannelReg
) {
	blmorefx_tab[uwCmd >> 8](uwCmd, pChannelData, pChannelReg);
}

static void mt_updatefunk(tChannelStatus *pChannelData) {
	UBYTE ubVal = mt_FunkTable[pChannelData->n_funk];
	pChannelData->n_funkoffset += ubVal;
	if(pChannelData->n_funkoffset > 0) {
		return;
	}
	pChannelData->n_funkoffset = 0;
	UBYTE *pOffs = (UBYTE*)&pChannelData->n_loopstart[pChannelData->n_replen];
	UBYTE *pWaveStart = &pChannelData->n_wavestart[1];
	if(pWaveStart >= pOffs) {
		pWaveStart = pOffs;
	}
	pChannelData->n_wavestart = pWaveStart;
	*pWaveStart = ~*pWaveStart;
}

static void checkmorefx(
	UWORD uwCmd, UWORD uwCmdArg,
	tChannelStatus *pChannelData, volatile tChannelRegs *pChannelReg
) {
	if(pChannelData->n_funk) {
		mt_updatefunk(pChannelData);
	}
	morefx_tab[uwCmd](uwCmdArg, pChannelData, pChannelReg);
}

static void mt_playvoice(
	tChannelStatus *pChannelData, volatile tChannelRegs *pChannelReg,
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
			!(pChannelData->n_intbit & s_uwAudioInterrupts) ||
			(pChannelData->n_dmabit & mt_dmaon)
		) {
			moreBlockedFx(pVoice->uwCmd, pChannelData, pChannelReg);
			return;
		}
		// sound effect sample has played, so unblock this channel again
		pChannelData->n_sfxpri = 0;
	}
	// n_note/cmd: any note or cmd set?
	if(!pChannelData->n_note && !pChannelData->n_cmd && !pChannelData->n_cmdlo) {
		pChannelReg->ac_per = pChannelData->n_period;
	}
	pChannelData->n_note = pVoice->uwNote;
	pChannelData->n_cmd = pVoice->ubCmdHi;
	pChannelData->n_cmdlo = pVoice->ubCmdLo;

	UWORD uwCmd = pChannelData->n_cmd & 0x000F;
	UWORD uwCmdArg = pVoice->ubCmdLo;
	UWORD uwMaskedCmdE = pVoice->uwCmd & 0x0FF0;

	// Get sample start address from cmd/note - BA is sample number
	// A...B... -> ......BA
	UWORD uwSampleIdx = ((pVoice->uwNote & 0xF000) >> 8) | ((pVoice->ubCmdHi & 0xF0) >> 4);
	if(uwSampleIdx) {
		// Samples are internally zero-based, in file 1-based
		--uwSampleIdx;
		// Read length, volume and repeat from sample info table
		UWORD *pSampleStart = mt_SampleStarts[uwSampleIdx];
		tModSampleHeader *pSampleDef = &mt_mod->pSamples[uwSampleIdx];
		UWORD uwSampleLength = pSampleDef->uwLength;
		if(!uwSampleLength) {
			// Use the first two bytes from the first sample for empty samples
			pSampleStart = mt_SampleStarts[0];
			uwSampleLength = 1;
		}
		// logWrite("playvoice sample: %hu, length: %hu\n", uwSampleIdx, uwSampleLength);
		pChannelData->n_start = pSampleStart;
		pChannelData->n_reallength = uwSampleLength;

		// Determine period table from fine-tune parameter
		UBYTE ubFineTune = pSampleDef->ubFineTune; // d3
		pChannelData->n_pertab = mt_PeriodTables[ubFineTune];
		pChannelData->n_minusft = (ubFineTune >= 8);

		pChannelData->n_volume = pSampleDef->ubVolume;
		if(!pSampleDef->uwRepeatOffs) {
			pChannelData->n_looped = 0;
			pChannelData->n_replen = pSampleDef->uwLength;
		}
		else {
			// Set repeat
			pChannelData->n_looped = 1;
			pChannelData->n_replen = pSampleDef->uwRepeatLength;
			uwSampleLength = pSampleDef->uwRepeatLength + ubFineTune * 2;
			pSampleStart += pSampleDef->uwRepeatOffs;
		}

		// set_loop:
		pChannelData->n_length = uwSampleLength;
		pChannelData->n_loopstart = pSampleStart;
		pChannelData->n_wavestart = (UBYTE*)pSampleStart;
		pChannelReg->ac_vol = mt_MasterVolTab[pSampleDef->ubVolume];
	}

	// inlined set_regs function here:
	if(!pVoice->uwNote) {
		checkmorefx(uwCmd, uwCmdArg, pChannelData, pChannelReg);
	}
	else if(uwMaskedCmdE == 0x0E50) {
		set_finetune(uwCmd, uwCmdArg, uwMaskedCmdE, pVoice, pChannelData, pChannelReg);
	}
	else {
		// logWrite("call cmd %hhu from table\n", uwCmd);
		prefx_tab[uwCmd](
			uwCmd, uwCmdArg, uwMaskedCmdE, pVoice, pChannelData, pChannelReg
		);
	}
}

static void mt_checkfx(
	tChannelStatus *pChannelData, volatile tChannelRegs *pChannelReg
) {
	if(pChannelData->n_sfxpri) {
		UWORD uwLen = pChannelData->n_sfxlen;
		if(uwLen) {
			startSfx(uwLen, pChannelData, pChannelReg);
		}
		if(uwLen || (
			!(pChannelData->n_intbit & s_uwAudioInterrupts) ||
			(pChannelData->n_dmabit & g_pCustom->dmaconr)
		)) {
			// Channel is blocked, only check some E-commands
			UWORD uwCmd = pChannelData->n_cmd & 0x0FFF;
			if((uwCmd & 0xF00) == 0xE00) {
				// NOP command
			}
			else {
				blocked_e_cmds(uwCmd, pChannelData, pChannelReg);
			}
			return;
		}
		else {
			// sound effect sample has played, so unblock this channel again
			pChannelData->n_sfxpri = 0;
		}
	}
	// do channel effects between notes
	if(pChannelData->n_funk) {
		mt_updatefunk(pChannelData);
	}
	// TODO later: remove this OR by storing those values in a better way
	UWORD uwCmd = (pChannelData->n_cmd | pChannelData->n_cmdlo) & 0x0FFF;
	if(!uwCmd) {
		// Just set the current period - same as mt_pernop
		pChannelReg->ac_per = pChannelData->n_period;
	}
	else {
		UBYTE ubCmdIndex = (pChannelData->n_cmd & 0xF);
		fx_tab[ubCmdIndex](pChannelData->n_cmdlo, pChannelData, pChannelReg);
	}
}

static void mt_sfxonly(void);

static void intPlay(volatile tCustom *pCustom) {
	// it was a TA interrupt, do music when enabled
	if(mt_Enable) {
		// music with sfx inserted
		pCustom->color[0] = 0xF00;
		mt_music();
		pCustom->color[0] = 0x000;
	}
	else {
		// no music, only sfx
		mt_sfxonly();
	}
}

// TimerA interrupt calls _mt_music at a selectable tempo (Fxx command),
// which defaults to 50 times per second.
static void mt_TimerAInt(
	REGARG(volatile tCustom *pCustom, "a0"),
	UNUSED_ARG REGARG(volatile void *pData, "a1")
) {
#if defined(PTPLAYER_DEFER_INTERRUPTS)
	s_isPendingPlay = 1;
#else
	intPlay(pCustom);
#endif
}

static inline void ptplayerEnableMainHandler(UBYTE isEnabled) {
#if defined(PTPLAYER_USE_VBL)
	if(isEnabled) {
		systemSetInt(INTB_VERTB, mt_TimerAInt, 0);
	}
	else {
		systemSetInt(INTB_VERTB, 0, 0);
	}
#else
	if(isEnabled) {
		systemSetCiaInt(CIA_B, CIAICRB_TIMER_A, mt_TimerAInt, 0);
	}
	else {
		systemSetCiaInt(CIA_B, CIAICRB_TIMER_A, 0, 0);
	}
#endif
}

static void intSetRep(volatile tCustom *pCustom) {
	pCustom->color[0] = 0x0F0;
	// check and clear CIAB interrupt flags
	// clear EXTER and possible audio interrupt flags
	// KaiN's note: Audio DMAs are 0..3 whereas INTs are (0..3) << 7
	s_uwAudioInterrupts = 0;

	// logWrite(
	// 	"set replen: %p %hu, %p %hu, %p %hu, %p %hu\n",
	// 	mt_chan[0].n_loopstart, mt_chan[0].n_replen,
	// 	mt_chan[1].n_loopstart, mt_chan[1].n_replen,
	// 	mt_chan[2].n_loopstart, mt_chan[2].n_replen,
	// 	mt_chan[3].n_loopstart, mt_chan[3].n_replen
	// );

	// Set repeat sample pointers and lengths
	pCustom->aud[0].ac_ptr = mt_chan[0].n_loopstart;
	pCustom->aud[0].ac_len = mt_chan[0].n_replen;
	pCustom->aud[1].ac_ptr = mt_chan[1].n_loopstart;
	pCustom->aud[1].ac_len = mt_chan[1].n_replen;
	pCustom->aud[2].ac_ptr = mt_chan[2].n_loopstart;
	pCustom->aud[2].ac_len = mt_chan[2].n_replen;
	pCustom->aud[3].ac_ptr = mt_chan[3].n_loopstart;
	pCustom->aud[3].ac_len = mt_chan[3].n_replen;

	// restore TimerA music interrupt vector
	ptplayerEnableMainHandler(1);
	systemSetCiaInt(CIA_B, CIAICRB_TIMER_B, 0, 0);
	pCustom->color[0] = 0x000;
}

// One-shot TimerB interrupt to set repeat samples after another 576 ticks.
static void mt_TimerBsetrep(
	REGARG(volatile tCustom *pCustom, "a0"),
	UNUSED_ARG REGARG(volatile void *pData, "a1")
) {
#if defined(PTPLAYER_DEFER_INTERRUPTS)
	s_isPendingSetRep = 1;
#else
	intSetRep(pCustom);
#endif
}

static void intDmaOn(volatile tCustom *pCustom) {
	pCustom->color[0] = 0x00F;
	// Restart timer to set repeat, enable DMA
	g_pCia[CIA_B]->crb = CIACRB_LOAD | CIACRB_RUNMODE | CIACRB_START;
	systemSetDmaMask(mt_dmaon, 1);

	// set level 6 interrupt to mt_TimerBsetrep
	ptplayerEnableMainHandler(0);
	systemSetCiaInt(CIA_B, CIAICRB_TIMER_B, mt_TimerBsetrep, 0);
	pCustom->color[0] = 0x000;
}

// One-shot TimerB interrupt to enable audio DMA after 576 ticks.
static void mt_TimerBdmaon(
	REGARG(volatile tCustom *pCustom, "a0"),
	UNUSED_ARG REGARG(volatile void *pData, "a1")
) {
#if defined(PTPLAYER_DEFER_INTERRUPTS)
	s_isPendingDmaOn = 1;
#else
	intDmaOn(pCustom);
#endif
}

static void chan_sfx_only(
	volatile tChannelRegs *pChannelReg, tChannelStatus *pChannelData
) {
	if(pChannelData->n_sfxpri <= 0) {
		return;
	}
	startSfx(pChannelData->n_sfxlen, pChannelData, pChannelReg);

	if(
		(pChannelData->n_intbit & s_uwAudioInterrupts) &&
		(pChannelData->n_dmabit & mt_dmaon)
	) {
		// Last sound effect sample has played, so unblock this channel again
		pChannelData->n_sfxpri = 0;
	}
}

// Called from interrupt.
// Plays sound effects on free channels.
void mt_sfxonly(void) {
	mt_dmaon = 0;
	chan_sfx_only(&g_pCustom->aud[0], &mt_chan[0]);
	chan_sfx_only(&g_pCustom->aud[1], &mt_chan[1]);
	chan_sfx_only(&g_pCustom->aud[2], &mt_chan[2]);
	chan_sfx_only(&g_pCustom->aud[3], &mt_chan[3]);

	if(mt_dmaon) {
		ptplayerEnableMainHandler(0);
		systemSetCiaInt(CIA_B, CIAICRB_TIMER_B, mt_TimerBdmaon, 0);
		g_pCia[CIA_B]->crb = CIACRB_LOAD | CIACRB_RUNMODE | CIACRB_START; // load/start timer B, one-shot
	}
}

// The replayer routine. Is called automatically after ptplayerStartPlayback().
// Called from interrupt.
// Play next position when Counter equals Speed.
// Effects are always handled.
void mt_music(void) {
	mt_dmaon = 0;
	if(++mt_Counter < mt_Speed) {
		// no new note, just check effects, don't step to next position
		mt_checkfx(&mt_chan[0], &g_pCustom->aud[0]);
		mt_checkfx(&mt_chan[1], &g_pCustom->aud[1]);
		mt_checkfx(&mt_chan[2], &g_pCustom->aud[2]);
		mt_checkfx(&mt_chan[3], &g_pCustom->aud[3]);

		// set one-shot TimerB interrupt for enabling DMA, when needed
		if(mt_dmaon) {
			ptplayerEnableMainHandler(0);
			systemSetCiaInt(CIA_B, CIAICRB_TIMER_B, mt_TimerBdmaon, 0);
			g_pCia[CIA_B]->crb = CIACRB_LOAD | CIACRB_RUNMODE | CIACRB_START; // load/start timer B, one-shot
		}
	}
	else {
		// handle a new note
		mt_Counter = 0;
		if(mt_PattDelTime2 <= 0) {
			// determine pointer to current pattern line
			UBYTE *pPatternData = &((UBYTE*)mt_mod)[sizeof(tModFileHeader)];
			UBYTE *pArrangement = mt_mod->pArrangement;
			UBYTE ubPatternIdx = pArrangement[mt_SongPos];
			UBYTE *pCurrentPattern = &pPatternData[ubPatternIdx * 1024];
			tModVoice *pLineVoices = (tModVoice*)&pCurrentPattern[mt_PatternPos];
			// logWrite(
			// 	"new note (cmd note): %04X %04X, %04X %04X, %04X %04X, %04X %04X\n",
			// 	pLineVoices[0].uwCmd, pLineVoices[0].uwNote,
			// 	pLineVoices[1].uwCmd, pLineVoices[1].uwNote,
			// 	pLineVoices[2].uwCmd, pLineVoices[2].uwNote,
			// 	pLineVoices[3].uwCmd, pLineVoices[3].uwNote
			// );

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
		if(mt_dmaon) {
			ptplayerEnableMainHandler(0);
			systemSetCiaInt(CIA_B, CIAICRB_TIMER_B, mt_TimerBdmaon, 0);
			g_pCia[CIA_B]->crb = CIACRB_LOAD | CIACRB_RUNMODE | CIACRB_START; // load/start timer B, one-shot
		}

		// next pattern line, handle delay and break
		mt_SilCntValid = 0; // recalculate silence counters
		UBYTE ubOffs = 16; // Offset to next pattern line
		UBYTE ubPatternDelay = mt_PattDelTime2;
		if(mt_PattDelTime) {
			ubPatternDelay = mt_PattDelTime;
			mt_PattDelTime = 0;
		}
		// logWrite("pattern delay: %hhu\n", ubPatternDelay);
		if(ubPatternDelay) {
			--ubPatternDelay;
			if(ubPatternDelay) {
				ubOffs = 0; // Do not advance to next line
			}
			else {
				// logWrite("Advance to next line\n");
			}
			mt_PattDelTime2 = ubPatternDelay;
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
void ptplayerEnd(void) {
	ptplayerEnableMusic(0);
	g_pCustom->aud[0].ac_vol = 0;
	g_pCustom->aud[1].ac_vol = 0;
	g_pCustom->aud[2].ac_vol = 0;
	g_pCustom->aud[3].ac_vol = 0;
	systemSetDmaMask(DMAF_AUDIO, 0);
}

void mt_reset(void) {
	// reset speed and counters
	mt_Speed = 6;
	mt_Counter = 0;
	mt_PatternPos = 0;

	// Disable the filter
	g_pCia[CIA_A]->pra |= BV(1);

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
	ptplayerEnd();
}

static inline void setTempo(UWORD uwTempo) {
#if !defined(PTPLAYER_USE_VBL)
	systemSetTimer(CIA_B, 0, mt_timerval / uwTempo);
#endif
}

static void INTERRUPT onAudio(
	UNUSED_ARG REGARG(volatile tCustom *pCustom, "a0"),
	REGARG(volatile void *pData, "a1")
) {
	ULONG ulIntFlag = (ULONG)pData;
	s_uwAudioInterrupts |= ulIntFlag;
};

void ptplayerStopPlayback(void) {
	ptplayerEnd();
	// Disable handling of music
	ptplayerEnableMusic(0);
	systemSetCiaInt(CIA_B, 0, 0, 0);
	systemSetCiaInt(CIA_B, 1, 0, 0);
}

void ptplayerStartPlayback(UBYTE isPal) {
	ptplayerEnableMusic(0);
	s_uwAudioInterrupts = 0;
#if defined(PTPLAYER_DEFER_INTERRUPTS)
	s_isPendingDmaOn = 0;
	s_isPendingPlay = 0;
	s_isPendingSetRep = 0;
#endif

	// disable CIA B interrupts, set player interrupt vector
	g_pCustom->intena = INTF_EXTER;
	ptplayerEnableMainHandler(1);
	systemSetCiaInt(CIA_B, CIAICRB_TIMER_B, 0, 0);
	systemSetInt(INTB_AUD0, onAudio, (void*)INTF_AUD0);
	systemSetInt(INTB_AUD1, onAudio, (void*)INTF_AUD1);
	systemSetInt(INTB_AUD2, onAudio, (void*)INTF_AUD2);
	systemSetInt(INTB_AUD3, onAudio, (void*)INTF_AUD3);

	// determine if 02 clock for timers is based on PAL or NTSC
	if(isPal) {
		// Fcolor = 4.43361825 MHz (PAL color carrier frequency)
		// CPU Clock = Fcolor * 1.6 = 7.0937892 MHz
		// CIA Clock = Cpu Clock / 10 = 709.37892 kHz
		// 50 Hz Timer = CIA Clock / 50 = 14187.5784
		// Tempo num. = 50 Hz Timer*125 = 1773447
		mt_timerval = 1773447;
	}
	else {
		mt_timerval = 1789773;
	}

	//Load TimerA in continuous mode for the default tempo of 125
	setTempo(125);
#if !defined(PTPLAYER_USE_VBL)
	g_pCia[CIA_B]->cra = CIACRA_LOAD | CIACRA_START; // load timer, start continuous
#endif

	// Load TimerB with 576 ticks for setting DMA and repeat
	systemSetTimer(CIA_B, 1, 576);

	// Enable CIA B interrupts
	g_pCustom->intena = INTF_SETCLR | INTF_EXTER;

	mt_reset();
}

void ptplayerInit(UBYTE *pTrackerModule, UWORD *pSampleData, UWORD uwInitialSongPos) {
	logBlockBegin(
		"ptplayerInit(pTrackerModule: %p, pSampleData: %p, uwInitialSongPos: %hu)",
		pTrackerModule, pSampleData, uwInitialSongPos
	);

	// Initialize new module.
	// Reset speed to 6, tempo to 125 and start at given song position.
	// Master volume is at 64 (maximum).
	mt_mod = (tModFileHeader*)pTrackerModule;
	logWrite(
		"Song name: '%s', arrangement length: %hhu, end pos: %hhu\n",
		mt_mod->szSongName, mt_mod->ubArrangementLength, mt_mod->ubSongEndPos
	);

	// set initial song position
	if(uwInitialSongPos >= 950) {
		uwInitialSongPos = 0;
	}
	mt_SongPos = uwInitialSongPos;

	// sample data location is given?
	if(!pSampleData) {
		// Get number of highest pattern
		UBYTE ubLastPattern = 0;
		for(UBYTE i = 0; i < 127; ++i) {
			if(mt_mod->pArrangement[i] > ubLastPattern) {
				ubLastPattern = mt_mod->pArrangement[i];
			}
		}
		UBYTE ubPatternCount = ubLastPattern + 1;
		logWrite("Pattern count: %hhu\n", ubPatternCount);

		// now we can calculate the base address of the sample data
		ULONG ulSampleOffs = (
			sizeof(tModFileHeader) + ubPatternCount * MOD_PATTERN_LENGTH
		);
		pSampleData = (UWORD*)&pTrackerModule[ulSampleOffs];
		// FIXME: use as pointer for empty samples
	}

	// Save start address of each sample
	ULONG ulOffs = 0;
	logWrite("Sample data starts at: %p\n", pSampleData);
	for(UBYTE i = 0; i < 31; ++i) {
		mt_SampleStarts[i] = &pSampleData[ulOffs];
		if(mt_mod->pSamples[i].uwLength > 0) {
			logWrite(
				"Sample %hhu name: '%.*s', word length: %hu, start: %p, repeat offs: %hu, repeat len:%hu\n",
				i, 22, mt_mod->pSamples[i].szName, mt_mod->pSamples[i].uwLength,
				mt_SampleStarts[i], mt_mod->pSamples[i].uwRepeatOffs, mt_mod->pSamples[i].uwRepeatLength
			);
		}

		// make sure sample starts with two 0-bytes
		mt_SampleStarts[i][0] = 0;

		// go to next sample
		ulOffs += mt_mod->pSamples[i].uwLength;
	};

	mt_reset();
	logBlockEnd("ptplayerInit()");
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
		uwChannels &= s_uwAudioInterrupts;
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
	tChannelStatus *pChannelData, volatile tChannelRegs *pChannelReg
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
			const UWORD *pPeriodTable = pChannelData->n_pertab;
			UWORD uwNoteOffs = 0;
			UBYTE ubPeriodPos;
			for(ubPeriodPos = 0; ubPeriodPos < MOD_PERIOD_TABLE_LENGTH; ++ubPeriodPos) {
				if (uwNoteOffs >= pPeriodTable[ubPeriodPos]) {
					break;
				}
				uwNoteOffs += 2;
			}

			pChannelData->n_noteoff = uwNoteOffs;
			uwNew = pPeriodTable[ubPeriodPos];
		}
		pChannelReg->ac_per = uwNew;
	}
}

static void mt_vibrato_nc(
	tChannelStatus *pChannelData, volatile tChannelRegs *pChannelReg,
	UBYTE ubAmplitude, UBYTE ubSpeed
) {
	// calculate vibrato table offset
	UBYTE ubOffs = 64 * ubAmplitude + (pChannelData->n_vibratopos & 63);

	// select vibrato waveform
	const BYTE *pTable;
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
	UNUSED_ARG UBYTE ubArgs, UNUSED_ARG tChannelStatus *pChannelData,
	UNUSED_ARG volatile tChannelRegs *pChannelReg
) {

}

static void mt_arpeggio(
	UBYTE ubArgs, tChannelStatus *pChannelData,
	volatile tChannelRegs *pChannelReg
) {
	// uwCmd: 0x00'XY (x = first arpeggio offset, y = second arpeggio offset)
	static const BYTE pArpTab[] = {
		0, 1, -1, 0, 1, -1, 0, 1, -1, 0, 1, -1, 0, 1, -1, 0,
		1, -1, 0, 1, -1, 0, 1, -1, 0, 1, -1, 0, 1, -1, 0, 1
	};

	// Step 0, just use normal period
	pChannelReg->ac_per = pChannelData->n_period;

	UWORD uwVal;
	if(pArpTab[mt_Counter] >= 0) {
		// Step 1, arpeggio by left nibble
		uwVal = ubArgs >> 4;
	}
	else {
		// Step 2, arpeggio by right nibble
		uwVal = ubArgs & 0xF;
	}
	// offset current note
	uwVal *= 2;
	uwVal += pChannelData->n_noteoff;
	if(uwVal < 2 * 36) {
		// Set period with arpeggio offset from note table
		pChannelReg->ac_per = pChannelData->n_pertab[uwVal / 2];
		// TODO later: noteoff has byte offs from start of array, set it to divided
		// by 2 since we're using UWORD pointers, not UBYTE
	}
}

static void ptDoPortaUp(
	UBYTE ubVal, tChannelStatus *pChannelData,
	volatile tChannelRegs *pChannelReg
) {
	UWORD uwNewPer = MAX(113, pChannelData->n_period - ubVal);
	pChannelData->n_period = uwNewPer;
	pChannelReg->ac_per = uwNewPer;
}

static void mt_portaup(
	UBYTE ubArgs, tChannelStatus *pChannelData,
	volatile tChannelRegs *pChannelReg
) {
	// cmd 1 x x (subtract xx from period)
	ptDoPortaUp(ubArgs, pChannelData, pChannelReg);
}

static void ptDoPortaDn(
	UBYTE ubVal, tChannelStatus *pChannelData,
	volatile tChannelRegs *pChannelReg
) {
	UWORD uwNewPer = MIN(pChannelData->n_period - ubVal, 856);
	pChannelData->n_period = uwNewPer;
	pChannelReg->ac_per = uwNewPer;
}

static void mt_portadown(
	UBYTE ubArgs, tChannelStatus *pChannelData,
	volatile tChannelRegs *pChannelReg
) {
	// cmd 2 x x (add xx to period)
	ptDoPortaDn(ubArgs, pChannelData, pChannelReg);
}

static void mt_toneporta(
	UBYTE ubArgs, tChannelStatus *pChannelData,
	volatile tChannelRegs *pChannelReg
) {
	// uwCmd: 0x03'XY (xy = tone portamento speed)
	if(ubArgs) {
		pChannelData->n_toneportspeed = ubArgs;
		pChannelData->n_cmdlo = 0;
	}
	mt_toneporta_nc(pChannelData, pChannelReg);
}

static void mt_vibrato(
	UBYTE ubArgs, tChannelStatus *pChannelData,
	volatile tChannelRegs *pChannelReg
) {
	// uwCmd: 0x04'XY (x = speed, y = amplitude)
	UBYTE ubAmplitude = ubArgs & 0xF;
	if(ubAmplitude) {
		pChannelData->n_vibratoamp = ubAmplitude;
	}
	else {
		ubAmplitude = pChannelData->n_vibratoamp;
	}
	UBYTE ubSpeed = ubArgs >> 4;
	if(ubSpeed) {
		pChannelData->n_vibratospd = ubSpeed;
	}
	else {
		ubSpeed = pChannelData->n_vibratospd;
	}
	mt_vibrato_nc(pChannelData, pChannelReg, ubAmplitude, ubSpeed);
}

static void ptVolSlide(
	BYTE bVolNew, tChannelStatus *pChannelData,
	volatile tChannelRegs *pChannelReg
) {
	bVolNew = CLAMP(bVolNew, 0, 64);
	pChannelData->n_volume = bVolNew;
	pChannelReg->ac_per = pChannelData->n_period;
	pChannelReg->ac_vol = mt_MasterVolTab[bVolNew];
}

static void mt_volumeslide(
	UBYTE ubArgs, tChannelStatus *pChannelData,
	volatile tChannelRegs *pChannelReg
) {
	// uwCmd: 0x0A'XY (x = volume-up, y = volume-down)
	UBYTE ubVolDn  = ubArgs & 0x0F;
	UBYTE ubVolUp = ubArgs >> 4;

	BYTE bVol = pChannelData->n_volume;
	if(!ubVolUp) {
		// Slide up, until 64
		ptVolSlide(bVol + ubVolUp, pChannelData, pChannelReg);
	}
	else {
		// Slide down, until 0
		ptVolSlide(bVol - ubVolDn, pChannelData, pChannelReg);
	}
}

static void mt_tonevolslide(
	UBYTE ubArgs, tChannelStatus *pChannelData,
	volatile tChannelRegs *pChannelReg
) {
	// uwCmd: 0x05'XY (x = volume up, y = volume down)
	mt_toneporta_nc(pChannelData, pChannelReg);

	// Do a volume slide with current cmd's args
	mt_volumeslide(ubArgs, pChannelData, pChannelReg);
}

static void mt_vibrvolslide(
	UBYTE ubArgs, tChannelStatus *pChannelData,
	volatile tChannelRegs *pChannelReg
) {
	// uwCmd: 0x06'XY (x = volume-up, y = volume-down)

	// Do vibrato with previously stored args
	mt_vibrato_nc(
		pChannelData, pChannelReg,
		pChannelData->n_vibratoamp, pChannelData->n_vibratospd
	);

	// Do a volume slide with current cmd's args
	mt_volumeslide(ubArgs, pChannelData, pChannelReg);
}

static void mt_tremolo(
	UBYTE ubArgs, tChannelStatus *pChannelData,
	volatile tChannelRegs *pChannelReg
) {
	// uwCmd: 0x07'XY (x = speed, y = amplitude)
	UBYTE ubAmplitude = ubArgs & 0x0F;
	UBYTE ubSpeed = ubArgs >> 4;
	if(ubAmplitude) {
		pChannelData->n_tremoloamp = ubAmplitude;
	}
	else {
		ubAmplitude = pChannelData->n_tremoloamp;
	}
	if(ubSpeed) {
		pChannelData->n_tremolospd = ubSpeed;
	}
	else {
		ubSpeed = pChannelData->n_tremolospd;
	}

	// calculate tremolo table offset
	UWORD uwOffset = 64 * ubAmplitude + (pChannelData->n_tremolopos & 63);

	// select tremolo waveform
	UBYTE ubWaveformIdx = pChannelData->n_tremoloctrl & 3;
	const BYTE *pWaveform;
	if(ubWaveformIdx == 0) {
		// ctrl 0 selects a sine tremolo
		pWaveform = mt_VibratoSineTable;
	}
	else if(ubWaveformIdx == 1) {
		// ctrl 1 selects a sawtooth tremolo
		pWaveform = mt_VibratoSawTable;
	}
	else {
		// ctrl 2 & 3 select a rectangle tremolo
		pWaveform = mt_VibratoRectTable;
	}

	// add tremolo-offset to volume
	WORD wNewVol = pChannelData->n_volume + pWaveform[uwOffset];
	wNewVol = CLAMP(wNewVol, 0, 64);

	pChannelReg->ac_per = pChannelData->n_period;
	pChannelReg->ac_vol = wNewVol;

	// increase tremolopos by speed
	pChannelData->n_tremolopos += ubSpeed;
}

static void mt_e_cmds(
	UBYTE ubArgs, tChannelStatus *pChannelData,
	volatile tChannelRegs *pChannelReg
) {
	// uwCmd: 0x0E'XY (x = command, y = argument)
	UBYTE ubArgE = ubArgs & 0x0F;
	UBYTE ubCmdE = (ubArgs & 0xF0) >> 4;
	ecmd_tab[ubCmdE](ubArgE, pChannelData, pChannelReg);
}

/**
 * @brief Protracker commands
 */
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

static void mt_posjump(
	UBYTE ubArgs, UNUSED_ARG tChannelStatus *pChannelData,
	UNUSED_ARG volatile tChannelRegs *pChannelReg
) {
	// uwCmd: 0x0B'XY (xy = new song position)
	mt_SongPos = ubArgs - 1;
	mt_PBreakPos = 0;
	mt_PosJumpFlag = 1;
}

static void mt_patternbrk(
	UBYTE ubArgs, UNUSED_ARG tChannelStatus *pChannelData,
	UNUSED_ARG volatile tChannelRegs *pChannelReg
) {
	// uwCmd: 0x0D'XY (xy = break pos in decimal)

	static const UBYTE pMult10[] = {
		0, 10, 20, 30, 40, 50, 60, 70, 80, 90, 0, 0, 0, 0, 0, 0
	};

	UBYTE ubY = ubArgs & 0xF;
	UBYTE ubX = ubArgs >> 4;
	UBYTE ubVal = pMult10[ubX] + ubY;

	if(ubVal > 63) {
		mt_PBreakPos = 0;
	}
	else {
		mt_PBreakPos = ubVal << 4;
	}
	mt_PosJumpFlag = 1;
}

static void blocked_e_cmds(
	UBYTE ubArgs, tChannelStatus *pChannelData,
	volatile tChannelRegs *pChannelReg
) {
	// uwCmd: 0x0E'XY (x = command, y = argument)
	UBYTE ubArg = ubArgs & 0x0F;
	UBYTE ubCmdE = ubArgs & 0xF0 >> 4;
	blecmd_tab[ubCmdE](ubArg, pChannelData, pChannelReg);
}

static void mt_setspeed(
	UBYTE ubArgs, UNUSED_ARG tChannelStatus *pChannelData,
	UNUSED_ARG volatile tChannelRegs *pChannelReg
) {
	// uwCmd: 0x0F'XY (xy < 0x20: new speed, xy >= 0x20: new tempo)
	if(ubArgs < 0x20) {
		mt_Speed = ubArgs;
		if(!ubArgs) {
			ptplayerEnd();
		}
	}
	else {
		// Set tempo (CIA only)
		setTempo(ubArgs);
	}
}

static void mt_pernop(
	UNUSED_ARG UBYTE ubArgs,
	tChannelStatus *pChannelData, volatile tChannelRegs *pChannelReg
) {
	// just set the current period
	pChannelReg->ac_per = pChannelData->n_period;
}

static void mt_volchange(
	UBYTE ubNewVolume,
	UNUSED_ARG tChannelStatus *pChannelData, volatile tChannelRegs *pChannelReg
) {
	// cmd C x y (xy = new volume)
	if(ubNewVolume > 64) {
		ubNewVolume = 64;
	}
	pChannelReg->ac_vol = mt_MasterVolTab[ubNewVolume];
}

static const tFx blmorefx_tab[16] = {
	[0 ... 10] = mt_nop,
	[11] = mt_posjump, // 0xB
	[13] = mt_patternbrk, // 0xD
	[14] = blocked_e_cmds,
	[15] = mt_setspeed, // 0xF
};

static const tFx morefx_tab[16] = {
	[0 ... 10] = mt_pernop,
	[11] = mt_posjump,
	[12] = mt_volchange,
	[13] = mt_patternbrk,
	[14] = mt_e_cmds,
	[15] = mt_setspeed
};

//----------------------------------------------------------------- E CMDS TABLE

static void mt_filter(
	UBYTE ubArg, UNUSED_ARG tChannelStatus *pChannelData,
	UNUSED_ARG volatile tChannelRegs *pChannelReg
) {
	// cmd 0x0E'0X (x=1 disable, x=0 enable)
	if(ubArg & 1) {
		g_pCia[CIA_A]->pra |= BV(1);
	}
	else {
		g_pCia[CIA_A]->pra &= ~BV(1);
	}
}

static void mt_fineportaup(
	UBYTE ubArg, tChannelStatus *pChannelData,
	volatile tChannelRegs *pChannelReg
) {
	// cmd 0x0E'1X (subtract x from period)
	if(!mt_Counter) {
		ptDoPortaUp(ubArg, pChannelData, pChannelReg);
	}
}

static void mt_fineportadn(
	UBYTE ubArg, tChannelStatus *pChannelData,
	volatile tChannelRegs *pChannelReg
) {
	// cmd 0x0E'1X (subtract x from period)
	if(!mt_Counter) {
		ptDoPortaDn(ubArg, pChannelData, pChannelReg);
	}
}

static void mt_glissctrl(
	UBYTE ubArg, tChannelStatus *pChannelData,
	UNUSED_ARG volatile tChannelRegs *pChannelReg
) {
	// cmd 0x0E'3X (x = gliss)
	pChannelData->n_gliss = (pChannelData->n_gliss & 4) | ubArg;
}

static void mt_vibratoctrl(
	UBYTE ubArg, tChannelStatus *pChannelData,
	UNUSED_ARG volatile tChannelRegs *pChannelReg
) {
	// cmd 0x0E'4X (x = vibrato)
	pChannelData->n_vibratoctrl = ubArg;
}

static void mt_finetune(
	UBYTE ubArg, tChannelStatus *pChannelData,
	UNUSED_ARG volatile tChannelRegs *pChannelReg
) {
	// cmd 0x0E'5X (x = finetune)
	pChannelData->n_pertab = mt_PeriodTables[ubArg];
	pChannelData->n_minusft = (ubArg >= 8);
}

static void mt_jumploop(
	UBYTE ubArg, tChannelStatus *pChannelData,
	UNUSED_ARG volatile tChannelRegs *pChannelReg
) {
	// cmd 0x0E'6X (x = 0: loop start, else loop count)
	if(mt_Counter) {
		return;
	}
	if(!ubArg) {
		// remember start of loop position
		pChannelData->n_pattpos = mt_PatternPos;
		return;
	}
	// otherwise we are at the end of the loop
	--pChannelData->n_loopcount;
	if(!pChannelData->n_loopcount) {
		// loop finished
		return;
	}
	else if(pChannelData->n_loopcount < 0) {
		// initialize loop counter
		pChannelData->n_loopcount  = ubArg;
	}

	// jump back to start of loop
	mt_PBreakPos = pChannelData->n_pattpos;
	mt_PBreakFlag = 1;
}

static void mt_tremoctrl(
	UBYTE ubArg, tChannelStatus *pChannelData,
	UNUSED_ARG volatile tChannelRegs *pChannelReg
) {
	// cmd 0x0E'7X (x = tremolo)
	pChannelData->n_tremoloctrl = ubArg;
}

static void mt_e8(
	UBYTE ubArg, UNUSED_ARG tChannelStatus *pChannelData,
	UNUSED_ARG volatile tChannelRegs *pChannelReg
) {
	// cmd 0x0E'8X (x = trigger value)
	mt_E8Trigger = ubArg;
}

static void ptDoRetrigger(
	tChannelStatus *pChannelData, volatile tChannelRegs *pChannelReg
) {
	// DMA off, set sample pointer and length
	systemSetDmaMask(pChannelData->n_dmabit, 0);
	// logWrite("retrigger: %p:%hu\n", pChannelData->n_start, pChannelData->n_length);
	pChannelReg->ac_ptr = pChannelData->n_start;
	pChannelReg->ac_len = pChannelData->n_length;
	mt_dmaon |= pChannelData->n_dmabit;
}

static void mt_retrignote(
	UBYTE ubArg, tChannelStatus *pChannelData,
	volatile tChannelRegs *pChannelReg
) {
	// cmd 0x0E'9X (x = retrigger count)
	if(!ubArg) {
		return;
	}

	// set new retrigger count when Counter = 0
	if(!mt_Counter) {
		pChannelData->n_retrigcount = ubArg;
		// avoid double retrigger, when Counter=0 and a note was set
		if(pChannelData->n_note & 0x0FFF) {
			return;
		}
	}
	else {
		// check if retrigger count is reached
		--pChannelData->n_retrigcount;
		if(pChannelData->n_retrigcount) {
			return;
		}
		// reset
		pChannelData->n_retrigcount = ubArg;
	}

	ptDoRetrigger(pChannelData, pChannelReg);
}

static void mt_volfineup(
	UBYTE ubArg, tChannelStatus *pChannelData,
	volatile tChannelRegs *pChannelReg
) {
	// cmd 0x0E'AX (x = volume add)
	if(!mt_Counter) {
		ptVolSlide(pChannelData->n_volume + ubArg, pChannelData, pChannelReg);
	}
}

static void mt_volfinedn(
	UBYTE ubArg, tChannelStatus *pChannelData,
	volatile tChannelRegs *pChannelReg
) {
	// cmd 0x0E'BX (x = volume subtract)
	if(!mt_Counter) {
		ptVolSlide(pChannelData->n_volume - ubArg, pChannelData, pChannelReg);
	}
}

static void mt_notecut(
	UBYTE ubArg, tChannelStatus *pChannelData,
	volatile tChannelRegs *pChannelReg
) {
	// cmd 0x0E'CX (x = counter to cut at)
	if(mt_Counter == ubArg) {
		pChannelData->n_volume = 0;
		pChannelReg->ac_vol = 0;
	}
}

static void mt_notedelay(
	UBYTE ubArg, tChannelStatus *pChannelData,
	volatile tChannelRegs *pChannelReg
) {
	// cmd 0x0E'DX (x = counter to retrigger at)
	if(mt_Counter == ubArg) {
		// Trigger note when given
		if(pChannelData->n_note) {
			ptDoRetrigger(pChannelData, pChannelReg);
		}
	}
}

static void mt_patterndelay(
	UBYTE ubArg, UNUSED_ARG tChannelStatus *pChannelData,
	UNUSED_ARG volatile tChannelRegs *pChannelReg
) {
	// cmd 0x0E'EX (x = delay count)
	if(!mt_Counter && !mt_PattDelTime2) {
		mt_PattDelTime = ubArg + 1;
		// logWrite("set pattern delay: %hu\n", mt_PattDelTime);
	}
}

static void mt_funk(
	UBYTE ubArg, tChannelStatus *pChannelData,
	UNUSED_ARG volatile tChannelRegs *pChannelReg
) {
	// cmd 0x0E'FX (x = delay count)
	if(!mt_Counter) {
		pChannelData->n_funk = ubArg;
		if(ubArg) {
			mt_updatefunk(pChannelData);
		}
	}
}

static void mt_rts(
	UNUSED_ARG UBYTE ubArg, UNUSED_ARG tChannelStatus *pChannelData,
	UNUSED_ARG volatile tChannelRegs *pChannelReg
) {
	// NOP
}

static const tEFn ecmd_tab[16] = {
	mt_filter,
	mt_fineportaup,
	mt_fineportadn,
	mt_glissctrl,
	mt_vibratoctrl,
	mt_finetune,
	mt_jumploop,
	mt_tremoctrl,
	mt_e8,
	mt_retrignote,
	mt_volfineup,
	mt_volfinedn,
	mt_notecut,
	mt_notedelay,
	mt_patterndelay,
	mt_funk
};

static const tEFn blecmd_tab[16] = {
	mt_filter,
	mt_rts, mt_rts,
	mt_glissctrl,
	mt_vibratoctrl,
	mt_finetune,
	mt_jumploop,
	mt_tremoctrl,
	mt_e8,
	mt_rts, mt_rts, mt_rts, mt_rts, mt_rts
};

static void set_period(
	UWORD uwCmd, UWORD uwCmdArg, UWORD uwMaskedCmdE, const tModVoice *pVoice,
	tChannelStatus *pChannelData, volatile tChannelRegs *pChannelReg
) {
	// Find nearest period for a note value, then apply finetuning
	UWORD uwNote = pVoice->uwNote & 0xFFF;
	UBYTE ubPeriodPos;
	for(ubPeriodPos = 0; ubPeriodPos < MOD_PERIOD_TABLE_LENGTH; ++ubPeriodPos) {
		if(uwNote >= mt_PeriodTables[0][ubPeriodPos]) {
			break;
		}
	}

	// Apply finetuning, set period and note-offset
	UWORD uwPeriod = pChannelData->n_pertab[ubPeriodPos];
	pChannelData->n_period = uwPeriod;
	pChannelData->n_noteoff = ubPeriodPos * 2; // TODO later: convert to word offs (div by 2)

	// Check for notedelay
	// logWrite("cmd: %04X, masked E: %04X\n", uwCmd, uwMaskedCmdE);
	// Skip if notedelay
	if(uwMaskedCmdE != 0x0ED0) {
		// Disable DMA
		systemSetDmaMask(pChannelData->n_dmabit, 0);

		if(!BTST(pChannelData->n_vibratoctrl, 2)) {
			pChannelData->n_vibratopos = 0;
		}
		if(!BTST(pChannelData->n_tremoloctrl, 2)) {
			pChannelData->n_tremolopos = 0;
		}
		// logWrite(
		// 	"setperiod: ptr %p, len: %hu, period: %hu\n",
		// 	pChannelData->n_start, pChannelData->n_length, uwPeriod
		// );

		pChannelReg->ac_ptr = pChannelData->n_start;
		pChannelReg->ac_len = pChannelData->n_length;
		pChannelReg->ac_per = uwPeriod;
		mt_dmaon |= pChannelData->n_dmabit;
	}
	checkmorefx(uwCmd, uwCmdArg, pChannelData, pChannelReg);
}

static void set_finetune(
	UWORD uwCmd, UWORD uwCmdArg, UWORD uwMaskedCmdE, const tModVoice *pVoice,
	tChannelStatus *pChannelData, volatile tChannelRegs *pChannelReg
) {
	// logWrite("Set finetune\n");
	UBYTE ubFineTune = pChannelData->n_cmdlo & 0xF;
	pChannelData->n_pertab = mt_PeriodTables[ubFineTune];
	pChannelData->n_minusft = ubFineTune >= 8;
	set_period(uwCmd, uwCmdArg, uwMaskedCmdE, pVoice, pChannelData, pChannelReg);
}

static void set_sampleoffset(
	UWORD uwCmd, UWORD uwCmdArg, UWORD uwMaskedCmdE, const tModVoice *pVoice,
	tChannelStatus *pChannelData, volatile tChannelRegs *pChannelReg
) {
	// logWrite("set_sampleoffset\n");
	// cmd 9 x y (xy = offset in 256 bytes)
	// d4 = xy
	UWORD ubArg = uwCmdArg;
	if(!ubArg) {
		ubArg = pChannelData->n_sampleoffset;
	}
	else {
		pChannelData->n_sampleoffset = ubArg;
	}
	UWORD uwLength = ubArg << 7;
	if(uwLength < pChannelData->n_length) {
		pChannelData->n_length -= uwLength;
		pChannelData->n_start += uwLength;
	}
	else {
		pChannelData->n_length = 1;
	}
	set_period(uwCmd, uwCmdArg, uwMaskedCmdE, pVoice, pChannelData, pChannelReg);
}

static void set_toneporta(
	UNUSED_ARG UWORD uwCmd, UNUSED_ARG UWORD uwCmdArg,
	UNUSED_ARG UWORD uwMaskedCmdE, const tModVoice *pVoice,
	tChannelStatus *pChannelData, volatile tChannelRegs *pChannelReg
) {
	// Find first period which is less or equal the note in d6
	UWORD uwNote = pVoice->uwNote & 0xFFF;
	UBYTE ubPeriodPos;
	UWORD uwNoteOffs = 0;
	for(ubPeriodPos = 0; ubPeriodPos < MOD_PERIOD_TABLE_LENGTH; ++ubPeriodPos) {
		if (uwNote >= mt_PeriodTables[0][ubPeriodPos]) {
			break;
		}
		uwNoteOffs += 2;
	}

	if(pChannelData->n_minusft && ubPeriodPos) {
		// Negative fine tune? Then take the previous period.
		--ubPeriodPos;
		uwNoteOffs -= 2;
	}

	// Note offset in period table
	pChannelData->n_noteoff = uwNoteOffs;
	UWORD uwPeriod = pChannelData->n_period;
	UWORD uwNewPeriod = pChannelData->n_pertab[--ubPeriodPos];
	if(uwNewPeriod == uwPeriod) {
		uwNewPeriod = 0;
	}
	pChannelData->n_wantedperiod = uwNewPeriod;

	if(pChannelData->n_funk) {
		mt_updatefunk(pChannelData);
	}

	pChannelReg->ac_per = uwPeriod;
}

static const tPreFx prefx_tab[16] = {
	[0 ... 2] = set_period,
	[3] = set_toneporta,
	[4] = set_period,
	[5] = set_toneporta,
	[6 ... 8] = set_period,
	[9] = set_sampleoffset,
	[0xA ... 0xF] = set_period,
};

void ptplayerProcess(void) {
#if defined(PTPLAYER_DEFER_INTERRUPTS)
	if(s_isPendingPlay) {
		s_isPendingPlay = 0;
		intPlay(g_pCustom);
	}
	if(s_isPendingDmaOn) {
		s_isPendingDmaOn = 0;
		intDmaOn(g_pCustom);
	}
	if(s_isPendingSetRep) {
		s_isPendingSetRep = 0;
		intSetRep(g_pCustom);
	}
#endif
}

const tModVoice *ptplayerGetCurrentVoices(void) {
	UBYTE *pPatternData = &((UBYTE*)mt_mod)[sizeof(tModFileHeader)];
	UBYTE *pArrangement = mt_mod->pArrangement;
	UBYTE ubPatternIdx = pArrangement[mt_SongPos];
	UBYTE *pCurrentPattern = &pPatternData[ubPatternIdx * 1024];
	tModVoice *pLineVoices = (tModVoice*)&pCurrentPattern[mt_PatternPos];
	return pLineVoices;
}

void ptplayerGetVoiceProgress(UWORD *pCurr, UWORD *pMax) {
	*pCurr = mt_Counter;
	*pMax = mt_Speed;
}

void ptplayerEnableMusic(UBYTE isEnabled) {
	mt_Enable = isEnabled;
}

UBYTE ptplayerGetE8(void) {
	return mt_E8Trigger;
}

void ptplayerReserveChannelsForMusic(UBYTE ubChannelCount) {
	mt_MusicChannels = ubChannelCount;
}
