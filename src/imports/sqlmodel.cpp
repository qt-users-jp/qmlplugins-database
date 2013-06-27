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
#include <QtSql/QSqlDatabase>
#include <QtSql/QSqlDriver>
#include <QtSql/QSqlError>
#include <QtSql/QSqlRecord>
#include <QtSql/QSqlQuery>

class SqlModel::Private : public QObject
{
    Q_OBJECT
public:
    Private(SqlModel *parent);
    void init();

    QString selectSql() const;

private slots:
    void databaseChanged(Database *database);
    void openChanged(bool open);
    void select();

private:
    SqlModel *q;

public:
    QList<QVariantList> data;
    QHash<int, QByteArray> roleNames;
};

SqlModel::Private::Private(SqlModel *parent)
    : QObject(parent)
    , q(parent)
{
}

void SqlModel::Private::init()
{
    connect(q, SIGNAL(databaseChanged(Database*)), this, SLOT(databaseChanged(Database*)));
    connect(q, SIGNAL(selectChanged(bool)), this, SLOT(select()));
    if (!q->m_database) {
        q->database(qobject_cast<Database *>(q->QObject::parent()));
    }

    select();
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

    QSqlDatabase db = QSqlDatabase::database(q->m_database->connectionName());

    if (!data.isEmpty()) {
        q->beginRemoveRows(QModelIndex(), 0, data.count() - 1);
        data.clear();
        q->endRemoveRows();
        emit q->countChanged(data.count());
    }

    QSqlQuery query(db);
    query.prepare(q->m_query);
    foreach (const QVariant &param, q->m_params) {
        query.addBindValue(param);
    }

    if (!query.exec()) {
        qDebug() << Q_FUNC_INFO << __LINE__ << query.lastQuery() << query.boundValues() << query.lastError();
        return;
    }
    QSqlRecord record = query.record();

    for (int i = 0; i < record.count(); i++) {
        roleNames.insert(Qt::UserRole + i, record.fieldName(i).toUtf8());
    }

    while (query.next()) {
        QVariantList d;
        for (int i = 0; i < roleNames.keys().count(); i++) {
            d.append(query.value(i));
        }
        data.append(d);
    }

    if (!data.isEmpty()) {
        q->beginInsertRows(QModelIndex(), 0, data.count() - 1);
        q->endInsertRows();
        emit q->countChanged(data.count());
    }
}

SqlModel::SqlModel(QObject *parent)
    : QAbstractListModel(parent)
    , d(new Private(this))
    , m_database(0)
    , m_select(true)
{
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
    return d->roleNames;
}

int SqlModel::rowCount(const QModelIndex &parent) const
{
    return d->data.count();
}

QVariant SqlModel::data(const QModelIndex &index, int role) const
{
    if (role >= Qt::UserRole) {
        return d->data.at(index.row()).at(role - Qt::UserRole);
    }
    return QVariant();
}

int SqlModel::count() const
{
    return rowCount();
}

QVariantMap SqlModel::get(int index) const
{
    QVariantMap ret;
    QVariantList list = d->data.at(index);
    for (int i = 0; i < list.length(); i++) {
//        qDebug() << Q_FUNC_INFO << __LINE__ << i << QString::fromUtf8(d->roleNames.value(Qt::UserRole + i)) << list.at(i);
        ret.insert(QString::fromUtf8(d->roleNames.value(Qt::UserRole + i)), list.at(i));
    }
    return ret;
}

#include "sqlmodel.moc"
