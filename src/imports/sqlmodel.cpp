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

#include "sqlmodel.h"
#include "database.h"

#include <QtCore/QDebug>
#include <QtCore/QMetaObject>
#include <QtCore/QMetaProperty>
#include <QtCore/QStringList>
#include <QtCore/QTime>
#include <QtCore/QThread>
#include <QtCore/QReadWriteLock>
#include <QtCore/QReadLocker>
#include <QtCore/QWriteLocker>

#include <QtSql/QSqlDatabase>
#include <QtSql/QSqlDriver>
#include <QtSql/QSqlError>
#include <QtSql/QSqlRecord>
#include <QtSql/QSqlQuery>

#define DEBUG() qDebug() << Q_FUNC_INFO << __LINE__

class SqlModel::Private : public QObject
{
    Q_OBJECT
public:
    Private(SqlModel *parent);
    void init();

    QString selectSql() const;

signals:
    void updated();
    void timerChanged(int timer);

private slots:
    void databaseChanged(Database *database);
    void openChanged(bool open);
    void select();

private:
    SqlModel *q;

public:
    QThread *thread;
    QReadWriteLock lock;
    QSqlQuery query;
    QHash<int, QByteArray> roleNames;
    bool quit;
    int timer;
    int count;
};

SqlModel::Private::Private(SqlModel *parent)
    : QObject(parent)
    , q(parent)
    , thread(0)
    , quit(false)
    , timer(0)
    , count(0)
{
}

void SqlModel::Private::init()
{
    Qt::ConnectionType type = Qt::DirectConnection;
    if (q->m_async) {
        thread = new QThread(q);
        // TODO
//        connect(thread, &QThread::destroyed, [&](QObject *object) {
//        });
        this->setParent(0);
        this->moveToThread(thread);
        thread->start();
        type = Qt::QueuedConnection;
    }

    connect(q, SIGNAL(databaseChanged(Database*)), this, SLOT(databaseChanged(Database*)), type);
    connect(q, SIGNAL(selectChanged(bool)), this, SLOT(select()), type);
    connect(q, SIGNAL(queryChanged(QString)), this, SLOT(select()), type);
    connect(q, SIGNAL(paramsChanged(QVariantList)), this, SLOT(select()), type);
    connect(this, SIGNAL(updated()), q, SLOT(updated()), type);
    connect(this, SIGNAL(timerChanged(int)), q, SIGNAL(timerChanged(int)), type);

    if (!q->m_database) {
        q->database(qobject_cast<Database *>(q->QObject::parent()));
    }
    QMetaObject::invokeMethod(this, "select", type);
}

void SqlModel::Private::databaseChanged(Database *database)
{
    disconnect(this, SLOT(openChanged(bool)));
    if (database) {
        connect(database, SIGNAL(openChanged(bool)), this, SLOT(openChanged(bool)));
        openChanged(database->open());
    }
}

void SqlModel::Private::openChanged(bool open)
{
    if (open) {
        if (q->query().isEmpty()) {
            qWarning() << "query is empty.";
            return;
        }
        select();
    }
}

void SqlModel::Private::select()
{
    if (!q->m_select) return;
    if (!q->m_database || !q->m_database->open()) return;

    if (query.isActive()) {
        query.finish();
    }

    QSqlDatabase db = QSqlDatabase::database(q->m_database->connectionName());

    query = QSqlQuery(db);
    query.prepare(q->m_query);
    foreach (const QVariant &param, q->m_params) {
        query.addBindValue(param);
    }

    QTime time;
    time.start();
    if (!query.exec()) {
        qDebug() << Q_FUNC_INFO << __LINE__ << query.lastQuery() << query.boundValues() << query.lastError();
        updated();
        return;
    }
    timer = time.elapsed();
    emit timerChanged(timer);

    QSqlRecord record = query.record();

    for (int i = 0; i < record.count(); i++) {
        roleNames.insert(Qt::UserRole + i, record.fieldName(i).toUtf8());
    }

    emit updated();
}

SqlModel::SqlModel(QObject *parent)
    : QAbstractListModel(parent)
    , d(new Private(this))
    , m_database(0)
    , m_select(true)
    , m_async(false)
{
}

SqlModel::~SqlModel()
{
    if (m_async) {
        d->lock.lockForWrite();
        d->quit = true;
        d->lock.unlock();
        d->thread->quit();
        d->thread->wait();
        delete d->thread;
        delete d;
    }
}

void SqlModel::updated()
{
//    DEBUG() << "locking read";
    if (m_async) d->lock.lockForRead();
//    DEBUG() << "locked";

    if (d->count > 0) {
        beginRemoveRows(QModelIndex(), 0, d->count - 1);
        endRemoveRows();
    }
//    DEBUG() << "unlocking";
    if (m_async) d->lock.unlock();
//    DEBUG() << "unlocked";

//    DEBUG() << "locking write";
    if (m_async) d->lock.lockForWrite();
//    DEBUG() << "locked";

    if (d->query.isActive())
        d->count = d->query.size();
    else
        d->count = 0;

//    DEBUG() << "unlocking";
    if (m_async) d->lock.unlock();
//    DEBUG() << "unlocked";

//    DEBUG() << "locking read";
    if (m_async) d->lock.lockForRead();
//    DEBUG() << "locked";

    if (d->count > 0) {
        beginInsertRows(QModelIndex(), 0, d->count - 1);
        endInsertRows();
    }

    emit countChanged(d->count);

//    DEBUG() << "unlocking";
    if (m_async) d->lock.unlock();
//    DEBUG() << "unlocked";
}

void SqlModel::classBegin()
{
}

void SqlModel::componentComplete()
{
    d->init();
}

QHash<int, QByteArray> SqlModel::roleNames() const
{
//    DEBUG() << "locking read";
    if (m_async) d->lock.lockForRead();
//    DEBUG() << "locked";

    QHash<int, QByteArray> ret = d->roleNames;

//    DEBUG() << "unlocking";
    if (m_async) d->lock.unlock();
//    DEBUG() << "unlocked";

    return ret;
}

int SqlModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent)
//    DEBUG() << QThread::currentThread() << thread() << d->thread;

//    DEBUG() << "locking read";
    if (m_async) d->lock.lockForRead();
//    DEBUG() << "locked";

    int ret = d->count;

//    DEBUG() << "unlocking";
    if (m_async) d->lock.unlock();
//    DEBUG() << "unlocked";

//    DEBUG() << ret;
//    DEBUG() << QThread::currentThread() << thread();
    return ret;
}

QVariant SqlModel::data(const QModelIndex &index, int role) const
{
    QVariant ret;
    if (role >= Qt::UserRole) {
//        DEBUG() << "locking read";
        if (m_async) d->lock.lockForRead();
//        DEBUG() << "locked";

        if (d->query.seek(index.row())) {
            ret = d->query.value(role - Qt::UserRole);
        }

//        DEBUG() << "unlocking";
        if (m_async) d->lock.unlock();
//        DEBUG() << "unlocked";
    }
    return ret;
}

int SqlModel::timer() const
{
    int ret = 0;
//    DEBUG() << "locking read";
    if (m_async) d->lock.lockForRead();
//    DEBUG() << "locked";

    ret = d->timer;

//    DEBUG() << "unlocking";
    if (m_async) d->lock.unlock();
//    DEBUG() << "unlocked";

    return ret;
}

int SqlModel::count() const
{
    return rowCount();
}

QVariantMap SqlModel::get(int index) const
{
    QVariantMap ret;

//    DEBUG() << "locking read";
    if (m_async) d->lock.lockForRead();
//    DEBUG() << "locked";

    if (d->query.seek(index)) {
        for (int i = 0; i < d->roleNames.keys().length(); i++) {
            ret.insert(QString::fromUtf8(d->roleNames.value(Qt::UserRole + i)), d->query.value(i));
        }
    }

//    DEBUG() << "unlocking";
    if (m_async) d->lock.unlock();
//    DEBUG() << "unlocked";

    return ret;
}

#include "sqlmodel.moc"
