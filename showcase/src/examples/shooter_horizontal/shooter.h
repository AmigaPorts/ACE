/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef _SHOWCASE_EXAMPLES_SHOOTER_HORIZONTAL_SHOOTER_H_
#define _SHOWCASE_EXAMPLES_SHOOTER_HORIZONTAL_SHOOTER_H_

/**
 * @file shooter.h
 * @brief Horizontal shooter example, using sprite.h, multiplexedsprite.h and
 * advancedmultiplexedsprite.h together.
 *
 * Channels: player 0+1 (sprite.h, attached), player shots 2, 6 and 7
 * (multiplexed), enemy shots 3 (multiplexed), enemies 4+5 (advanced
 * multiplexed, attached). Raw copper mode, with one sprite pointer table
 * shared by both managers.
 */

void gsExampleShooterHorzCreate(void);

void gsExampleShooterHorzLoop(void);

void gsExampleShooterHorzDestroy(void);

#endif // _SHOWCASE_EXAMPLES_SHOOTER_HORIZONTAL_SHOOTER_H_
