/*
 * Copyright (C) 2015 Canonical Ltd
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Author : Arto Jalkanen <ajalkane@gmail.com>
 */

#ifndef QMTABPARSER_H
#define QMTABPARSER_H

#include <QObject>

struct QMtabEntry {
    QString fsName;
    QString dir;
    QString type;
    QString opts;
    int freq;
    int passno;
};

class QMtabParser : public QObject
{
    Q_OBJECT
    QString m_path;

public:
    explicit QMtabParser(const QString& path = QString(), QObject *parent = 0);
    ~QMtabParser();

    QList<QMtabEntry> parseEntries();

    inline const QString& path() { return m_path; }

private:
    /*!
     * \brief fsHasUserContent() consider Disk and Network file systems as valid
     * \param fs
     * \return TRUE if the filesystem is considered as having normal user files, FALSE when it is supposed to be system FS
     *
     * \sa \link http://en.wikipedia.org/wiki/List_of_file_systems
     */
    bool  fsHasUserContent(const struct QMtabEntry& fs);
};

#endif // QMTABPARSER_H



