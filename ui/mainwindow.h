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

#ifndef UI_MAINWINDOW_H
#define UI_MAINWINDOW_H

#include <QMainWindow>

namespace Ui {
class ToolBar;
class ThinSplitter;
class ServerView;
class RendererView;
class LyricsView;
class PreferencesDialog;

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    MainWindow(QWidget *p);
    virtual ~MainWindow();

public Q_SLOTS:
    void raise();

private Q_SLOTS:
    void showPreferences();
    void showAbout();
    void showLyrics(bool s);
    void preferencesDestroyed();

private:
    ToolBar *toolBar;
    ThinSplitter *splitter;
    ServerView *server;
    RendererView *renderer;
    LyricsView *lyrics;
    PreferencesDialog *preferences;
};
}

#endif
