/* Copyright (c) 2012 Silk Project.
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of the Silk nor the
 *       names of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL SILK BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "database.h"

#include <QtCore/QDebug>
#include <QtCore/QTimer>
#include <QtSql/QSqlDatabase>
#include <QtSql/QSqlError>
#include <QtQml/qqml.h>
#include <QtQml/QQmlContext>

class Database::Private : public QObject
{
    Q_OBJECT
public:
    Private(Database *parent);

private:
    Database *q;

public:
    QList<QObject *> contents;
    bool open;
};

Database::Private::Private(Database *parent)
    : QObject(parent)
    , q(parent)
    , open(false)
{
}

Database::Database(QObject *parent)
    : QObject(parent)
    , m_hostName("localhost")
    , d(new Private(this))
{
}

QQmlListProperty<QObject> Database::contents()
{
    return QQmlListProperty<QObject>(this, d->contents);
}


bool Database::open()
{
    if (!d->open) {
        if (m_databaseName.isNull()) return false;
        if (m_connectionName.isEmpty()) {
            QQmlContext *context = qmlContext(this);
            connectionName(context->nameForObject(this));
        }

        if (!QSqlDatabase::contains(m_connectionName)) {
            QSqlDatabase db = QSqlDatabase::addDatabase(m_type, m_connectionName);
            db.setHostName(m_hostName);
            db.setDatabaseName(m_databaseName);
            db.setUserName(m_userName);
            db.setPassword(m_password);
            if (db.open()) {
                open(true);
            } else {
                qDebug() << Q_FUNC_INFO << __LINE__ << db.lastError().text();
            }
        } else {
            open(QSqlDatabase::database(m_connectionName).isOpen());
        }
    }
    return d->open;
}

bool Database::isOpen() const
{
    return d->open;
}

void Database::open(bool open)
{
    if (d->open == open) return;
    d->open = open;
    emit openChanged(open);
}

bool Database::transaction()
{
    QSqlDatabase db = QSqlDatabase::database(m_connectionName);
    return db.transaction();
}

bool Database::commit()
{
    QSqlDatabase db = QSqlDatabase::database(m_connectionName);
    return db.commit();
}

bool Database::rollback()
{
    QSqlDatabase db = QSqlDatabase::database(m_connectionName);
    return db.rollback();
}

#include "database.moc"
