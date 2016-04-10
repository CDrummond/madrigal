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

#ifndef UI_RENDERER_VIEW_H
#define UI_RENDERER_VIEW_H

#include <QWidget>
#include <QIcon>

class QSortFilterProxyModel;
class QLabel;
class QPushButton;
class QStackedWidget;
namespace Upnp {
class DevicesModel;
}
namespace Ui {
class Action;
class ListView;
class NavButton;
class ViewToolBar;
class SqueezedTextLabel;

class RendererView : public QWidget {
Q_OBJECT
public:
    RendererView(QWidget *p);
    virtual ~RendererView();

    bool scrollQueue() const { return autoScrollQueue; }
    void setScrollQueue(bool s);

private:
    void setInfoLabel();

private Q_SLOTS:
    void updateItems();
    void setActive(const QModelIndex &idx);
    void rendererSelected(const QModelIndex &idx);
    void useFirst();
    void itemClicked(const QModelIndex &idx);
    void navSelected(const QModelIndex &idx);
    void navSelected(int id);
    void goBack();
    void updateStats(quint32 num, quint32 dur);
    void clearQueue();
    void removeSelectedTracks();
    void scrollTo(const QModelIndex &idx);

private:
    bool autoScrollQueue;
    Upnp::DevicesModel *model;
    ViewToolBar *toolbar;
    QStackedWidget *stack;
    NavButton *nav;
    ListView *renderers;
    ListView *queue;
    QSortFilterProxyModel *proxy;
    QLabel *infoLabel;
    QPushButton *cancelButton;
    Action *repeatAction;
    Action *shuffleAction;
    Action *clearAction;
    Action *removeAction;
    SqueezedTextLabel *queueInfo;
    QIcon backIcon;
    QIcon homeIcon;
};

}
#endif
