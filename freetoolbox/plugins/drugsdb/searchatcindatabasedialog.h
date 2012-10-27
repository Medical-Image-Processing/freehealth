/***************************************************************************
 *  The FreeMedForms project is a set of free, open source medical         *
 *  applications.                                                          *
 *  (C) 2008-2012 by Eric MAEKER, MD (France) <eric.maeker@gmail.com>      *
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
 *   Main Developper : Eric MAEKER, <eric.maeker@gmail.com>                *
 *   Contributors :                                                        *
 *       NAME <MAIL@ADDRESS.COM>                                           *
 ***************************************************************************/
#ifndef SEARCHATCINDATABASEDIALOG_H
#define SEARCHATCINDATABASEDIALOG_H

#include <QDialog>
class QModelIndex;

namespace DrugsDB {
class SearchAtcInDatabaseDialogPrivate;

namespace Ui {
    class SearchAtcInDatabaseDialog;
}

class SearchAtcInDatabaseDialog : public QDialog
{
    Q_OBJECT

public:
    explicit SearchAtcInDatabaseDialog(QWidget *parent = 0, const QString &term = QString::null);
    ~SearchAtcInDatabaseDialog();

    QStringList getSelectedCodes();
    QStringList getSelectedLabels();

private Q_SLOTS:
    void setFilter();
    void on_term_textChanged(const QString &text);
    void on_lang_currentIndexChanged(const QString &text);
    void on_tableView_activated(const QModelIndex &index);

protected:
    void changeEvent(QEvent *e);

private:
    Ui::SearchAtcInDatabaseDialog *ui;
    SearchAtcInDatabaseDialogPrivate *d;
};

}  // End namespace DrugsDbCreator

#endif // SEARCHATCINDATABASEDIALOG_H
