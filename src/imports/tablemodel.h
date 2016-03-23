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

#ifndef TABLEMODEL_H
#define TABLEMODEL_H

#include <QtCore/QAbstractListModel>

#include <QtQml/QQmlParserStatus>

class Database;

class TableModel : public QAbstractListModel, public QQmlParserStatus
{
    Q_OBJECT

    Q_PROPERTY(Database *database READ database WRITE database NOTIFY databaseChanged)
    Q_PROPERTY(QString tableName READ tableName WRITE tableName NOTIFY tableNameChanged)
    Q_PROPERTY(QString primaryKey READ primaryKey WRITE primaryKey NOTIFY primaryKeyChanged)
    Q_PROPERTY(QString condition READ condition WRITE condition NOTIFY conditionChanged)
    Q_PROPERTY(QString order READ order WRITE order NOTIFY orderChanged)
    Q_PROPERTY(int limit READ limit WRITE limit NOTIFY limitChanged)
    Q_PROPERTY(int offset READ offset WRITE offset NOTIFY offsetChanged)
    Q_PROPERTY(QVariantList params READ params WRITE params NOTIFY paramsChanged)
    Q_PROPERTY(int count READ count NOTIFY countChanged)
    Q_PROPERTY(bool select READ select WRITE select NOTIFY selectChanged)

    Q_INTERFACES(QQmlParserStatus)
public:
    explicit TableModel(QObject *parent = 0);

    int count() const;
    Q_INVOKABLE QVariantMap get(int index) const;
    Q_INVOKABLE QVariant insert(const QVariantMap &data);
    Q_INVOKABLE void update(const QVariantMap &data);
    Q_INVOKABLE bool remove(const QVariantMap &data);
    Q_INVOKABLE int remove();
//    void clear();

    virtual int rowCount(const QModelIndex &parent = QModelIndex()) const;
    virtual QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
    virtual QHash<int, QByteArray> roleNames() const;

    virtual void classBegin();
    virtual void componentComplete();

signals:
    void databaseChanged(Database *database);
    void tableNameChanged(const QString &tableName);
    void primaryKeyChanged(const QString &primaryKey);
    void conditionChanged(const QString &condition);
    void orderChanged(const QString &order);
    void offsetChanged(int offset);
    void limitChanged(int limit);
    void paramsChanged(const QVariantList &params);
    void countChanged(int count);
    void selectChanged(bool select);

private:
    class Private;
    Private *d;

#define ADD_PROPERTY(type, name, type2) \
public: \
    type name() const { return m_##name; } \
    void name(type name) { \
        if (m_##name == name) return; \
        m_##name = name; \
        emit name##Changed(name); \
    } \
private: \
    type2 m_##name;

    ADD_PROPERTY(Database *, database, Database *)
    ADD_PROPERTY(const QString &, tableName, QString)
    ADD_PROPERTY(const QString &, primaryKey, QString)
    ADD_PROPERTY(const QString &, condition, QString)
    ADD_PROPERTY(const QString &, order, QString)
    ADD_PROPERTY(int, limit, int)
    ADD_PROPERTY(int, offset, int)
    ADD_PROPERTY(const QVariantList &, params, QVariantList)
    ADD_PROPERTY(bool, select, bool)

#undef ADD_PROPERTY
};

#endif // TABLEMODEL_H
