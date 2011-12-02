/***************************************************************************
 *  The FreeMedForms project is a set of free, open source medical         *
 *  applications.                                                          *
 *  (C) 2008-2011 by Eric MAEKER, MD (France) <eric.maeker@gmail.com>      *
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
 *  along with this program (COPYING.FREEMEDFORMS file).                   *
 *  If not, see <http://www.gnu.org/licenses/>.                            *
 ***************************************************************************/
/***************************************************************************
 *   Main Developpers :                                                    *
 *       Eric MAEKER, MD <eric.maeker@gmail.com>                           *
 *   Contributors :                                                        *
 *       NAME <MAIL@ADRESS>                                                *
 ***************************************************************************/
#ifndef DATAPACK_SERVEREDITOR_H
#define DATAPACK_SERVEREDITOR_H

#include <datapackutils/datapack_exporter.h>
#include <QWidget>

/**
 * \file servereditor.h
 * \author Eric MAEKER <eric.maeker@gmail.com>
 * \version 0.6.2
 * \date 02 Dec 2011
*/

namespace DataPack {
namespace Ui {
    class ServerEditor;
}

class DATAPACK_EXPORT ServerEditor : public QWidget
{
    Q_OBJECT
public:
    explicit ServerEditor(QWidget *parent = 0);
    ~ServerEditor();

public Q_SLOTS:
    bool submitChanges();

private:
    Ui::ServerEditor *ui;
};

}  // End namespace DataPack

#endif // DATAPACK_SERVEREDITOR_H
