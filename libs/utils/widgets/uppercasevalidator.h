/***************************************************************************
 *  The FreeMedForms project is a set of free, open source medical         *
 *  applications.                                                          *
 *  (C) 2008-2016 by Eric MAEKER, MD (France) <eric.maeker@gmail.com>      *
 *  All rights reserved.                                                   *
 *                                                                         *
 *  This program is free software: you can redistribute it and/or modify   *
 *  it under the terms of the GNU General Public License as published by   *
 *  the Free Software Foundation, either version 3 of the License, or      *
 *  (at your option) any later version.                                    *
 *                                                                         *
 *  This program is distributed in the hope that it will be useful,        *
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of         *
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the          *
 *  GNU General Public License for more details.                           *
 *                                                                         *
 *  You should have received a copy of the GNU General Public License      *
 *  along with this program (COPYING file).                   *
 *  If not, see <http://www.gnu.org/licenses/>.                            *
 ***************************************************************************/
/***************************************************************************
 *   Main Developers:                                                     *
 *       Eric MAEKER, MD <eric.maeker@gmail.com>                           *
 *  Contributors:                                                          *
 *       NAME <MAIL@ADDRESS.COM>                                           *
 ***************************************************************************/
#ifndef UPPERCASEVALIDATOR_H
#define UPPERCASEVALIDATOR_H

#include <utils/global_exporter.h>
#include <QValidator>
QT_BEGIN_NAMESPACE
class QString;
QT_END_NAMESPACE

/**
 * \file uppercasevalidator.h
 * \author Eric MAEKER <eric.maeker@gmail.com>
 * \version 0.6.2
 * \date 22 Feb 2012
*/

/**
  \class Utils::UpperCaseValidator
  Use this validator in QLineEdit when you want the user input to always be uppercase.
*/

namespace Utils {

class UTILS_EXPORT UpperCaseValidator : public QValidator
{
public:
    UpperCaseValidator(QObject *parent);
    ~UpperCaseValidator();

    QValidator::State validate(QString &text, int &pos) const;
};

class UTILS_EXPORT CapitalizationValidator : public QValidator
{
public:
    CapitalizationValidator(QObject *parent);
    ~CapitalizationValidator();

    QValidator::State validate(QString &text, int &pos) const;
};

}

#endif // UPPERCASEVALIDATOR_H
