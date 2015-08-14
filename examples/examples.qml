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

import QtQuick 2.0
import me.qtquick.Database 0.1

Item {
    id: root
    width: 400
    height: 300

    Database {
        id: db
        connectionName: 'examples/database.qml'
        type: "QSQLITE"
        databaseName: ":memory:"
        connectOptions: "QSQLITE_BUSY_TIMEOUT=100"

        TableModel {
            id: table
            tableName: 'Chat'
            primaryKey: 'key'
            property int key
            property string value
        }

        SqlModel {
            id: select
            query: "SELECT COUNT(key) as keys FROM Chat WHERE value LIKE ?"
            params: ['%Qt%']
        }
    }

    Rectangle {
        id: header
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.margins: 2
        height: field.font.pixelSize * 1.5

        border.color: 'gray'
        border.width: 1

        TextInput {
            id: field
            anchors.fill: parent
            anchors.margins: 2
            focus: true


            Keys.onReturnPressed: {
                select.select = false
                table.insert({'value': field.text})
                field.text = ''
                select.select = true
            }
        }
    }

    Flickable {
        anchors.top: header.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: footer.top
        contentWidth: width
        contentHeight: contents.height
        Grid {
            id: contents
            width: parent.width
            flow: Grid.TopToBottom
            rows: table.count + 1
            Text {
                width: 100
                text: 'key'
            }

            Repeater {
                model: table
                Text {
                    text: model.key
                }
            }
            Text {
                text: 'value'
            }

            Repeater {
                model: table
                Text {
                    text: model.value
                    MouseArea {
                        anchors.fill: parent
                        onClicked: {
                            select.select = false
                            table.remove({'key': model.key})
                            select.select = true
                        }
                    }
                }
            }
        }
    }
    Text {
        id: footer
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        text: 'the number of values that contains "Qt" is %1.'.arg(select.count > 0 ? select.get(0).keys : '0')
    }

}
