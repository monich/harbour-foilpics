/*
 * Copyright (C) 2018 Jolla Ltd.
 * Copyright (C) 2018 Slava Monich <slava@monich.com>
 *
 * You may use this file under the terms of the BSD license as follows:
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 *   1. Redistributions of source code must retain the above copyright
 *      notice, this list of conditions and the following disclaimer.
 *   2. Redistributions in binary form must reproduce the above copyright
 *      notice, this list of conditions and the following disclaimer in
 *      the documentation and/or other materials provided with the
 *      distribution.
 *   3. Neither the name of Jolla Ltd nor the names of its contributors
 *      may be used to endorse or promote products derived from this
 *      software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef FOILPICS_SELECTION_H
#define FOILPICS_SELECTION_H

#include <QtQml>
#include <QAbstractItemModel>

// There is no way to clear the busy state. It's set when the items is
// scheduled to be removed from the source model.

class FoilPicsSelection : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QObject* model READ model WRITE setModel NOTIFY modelChanged)
    Q_PROPERTY(QString role READ role WRITE setRole NOTIFY roleChanged)
    Q_PROPERTY(bool duplicatesAllowed READ duplicatesAllowed WRITE setDuplicatesAllowed NOTIFY duplicatesAllowedChanged)
    Q_PROPERTY(int count READ selectionCount NOTIFY selectionCountChanged)
    Q_PROPERTY(int busyCount READ busyCount NOTIFY busyCountChanged)

public:
    explicit FoilPicsSelection(QObject* aParent = NULL);
    ~FoilPicsSelection();

    int selectionCount() const;
    int busyCount() const;

    bool duplicatesAllowed() const;
    void setDuplicatesAllowed(bool aValue);

    QAbstractItemModel* model() const;
    void setModel(QObject* aModel);

    QString role() const;
    void setRole(QString aRole);

    Q_INVOKABLE bool busy(QString aValue) const;

    Q_INVOKABLE bool selected(QString aValue) const;
    Q_INVOKABLE void select(QString aValue);
    Q_INVOKABLE void unselect(QString aValue);
    Q_INVOKABLE void toggleSelection(QString aValue);
    Q_INVOKABLE void clearSelection();

    Q_INVOKABLE void selectAll();
    Q_INVOKABLE QList<int> getSelectedRows();
    Q_INVOKABLE QList<int> makeSelectionBusy();

Q_SIGNALS:
    void modelChanged();
    void roleChanged();
    void duplicatesAllowedChanged();
    void selectionCountChanged();
    void busyCountChanged();
    void busyChanged(QString aKey);
    void selectionChanged(QString aKey);
    void selectionCleared();

private:
    class Private;
    Private* iPrivate;
};

QML_DECLARE_TYPE(FoilPicsSelection)

#endif // FOILPICS_SELECTION_H
