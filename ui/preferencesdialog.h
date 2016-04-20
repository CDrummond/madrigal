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

#ifndef UI_PREFERENCES_DIALOG
#define UI_PREFERENCES_DIALOG

#include <QDialog>
#include "config.h"

class QCheckBox;
class QDialogButtonBox;
class QAbstractButton;

namespace Ui {

class RendererView;
class PreferencesDialog : public QDialog {
Q_OBJECT

public:
    PreferencesDialog(QWidget *p, RendererView *rv);
    virtual ~PreferencesDialog() { }

private Q_SLOTS:
    void clearCache();
    void buttonClicked(QAbstractButton *btn);
    void save();

private:
    QDialogButtonBox *box;
    RendererView *rendererView;
    #if defined QT_QTDBUS_FOUND || defined Q_OS_MAC
    QCheckBox *showNotifications;
    #endif
    QCheckBox *autoScrollPlayQueue;
};

}

#endif
