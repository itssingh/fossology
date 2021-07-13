/*****************************************************************************
 * SPDX-License-Identifier: GPL-2.0
 * SPDX-FileCopyrightText: 2021 Sarita Singh <saritasingh.0425@gmail.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 ****************************************************************************/

#include "match.hpp"

/**
 * @brief constructor for match class intended to store copyright/holder information
 * @param matchName content for copyright/holder
 * @param startPosition start byte for matched_text
 * @param length  no. of bytes matched
 */

Match::Match(string matchName, string type, unsigned startPosition, unsigned length)
    : matchName(matchName),type(type), startPosition(startPosition), length(length) {}

/**
 * @brief constructor for match class intended to store license information
 * @param matchName spdx key for license
 * @param percentage  score for license
 * @param licenseFullName full name of license
 * @param textUrl reference text url for license
 * @param startPosition start byte for matched_text
 * @param length  no. of bytes matched
 */

Match::Match(string matchName, int percentage, string licenseFullName,
             string textUrl, unsigned startPosition, unsigned length)
    : matchName(matchName), percentage(percentage),
      licenseFullName(licenseFullName), textUrl(textUrl),
      startPosition(startPosition), length(length) {}

/**
 * Default destructor for match class
 */

Match::~Match() {}

/**
 * @brief get the match name
 * @return  matchName
 */

const string Match::getMatchName() const { return matchName; }

/**
 * @brief get the match type
 * @return type
 */
const string Match::getType() const { return type; }
/**
 * @brief get the percent match
 * @return percentage
 */
int Match::getPercentage() const { return percentage; }

/**
 * @brief get the license full name
 * @return  full name of matched license
 */

const string Match::getLicenseFullName() const { return licenseFullName; }

/**
 * @brief get the reference text URL
 * @return  reference text URL
 */

const string Match::getTextUrl() const { return textUrl; }

/**
 * @brief get the start byte of the matched text
 * @return  start position(byte)
 */

unsigned Match::getStartPosition() const { return startPosition; }

/**
 * @brief get the match text length
 * @return no. of byte matched
 */

unsigned Match::getLength() const { return length; }
