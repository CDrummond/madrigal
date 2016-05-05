/*
 * Madrigal
 *
 * Copyright (c) 2016 Craig Drummond <craig.p.drummond@gmail.com>
 *
 * ----
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#ifndef UI_SERVER_VIEW_H
#define UI_SERVER_VIEW_H

#include <QWidget>
#include <QIcon>

class QSortFilterProxyModel;
class QLabel;
class QPushButton;
class QStackedWidget;
class QLineEdit;
class QTimer;

namespace Upnp {
class DevicesModel;
}
namespace Ui {
class LibraryView;
class ListView;
class NavButton;
class ViewToolBar;
class FlatToolButton;
class AlbumInfo;
class Action;
class AnimatedIcon;

class ServerView : public QWidget {
Q_OBJECT
public:
    ServerView(QWidget *p);
    virtual ~ServerView();

private:
    void setInfoLabel();
    void showButtons();

private Q_SLOTS:
    void updateItems(bool activeSet=false);
    void setActive(const QModelIndex &idx);
    void serverSelected(const QModelIndex &idx);
    void useFirst();
    void itemClicked(const QModelIndex &idx);
    void doAction(int act);
    void navSelected(const QModelIndex &idx);
    void navSelected(int id);
    void goBack();
    void updateView(const QModelIndex &idx, bool force=false);
    void aboutToRemove(const QModelIndex &idx, int from, int to);
    void systemUpdated();
    void addAlbum();
    void playAlbum();
    void addSelection();
    void playSelection();
    void toggleSearch();
    void startSearchTimer();
    void doSearch();
    void searching(bool status);
    void controlSearch(bool enabled);

private:
    void addAlbumToQueue(bool andPlay);
    void setMediaIndex(const QModelIndex &idx);

private:
    Upnp::DevicesModel *model;
    ViewToolBar *toolbar;
    QStackedWidget *stack;
    NavButton *nav;
    ListView *servers;
    LibraryView *media;
    AlbumInfo *albumInfo;
    QSortFilterProxyModel *proxy;
    QLabel *infoLabel;
    QPushButton *cancelButton;
    FlatToolButton *searchButton;
    Action *searchAction;
    QLineEdit *searchText;
    QTimer *searchTimer;
    QColor iconColor;
    QIcon backIcon;
    AnimatedIcon *spinner;
};

}
#endif
