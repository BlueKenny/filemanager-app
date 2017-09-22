import QtQuick 2.4
import Ubuntu.Components 1.3
import Ubuntu.Components.Popups 1.3

import "../backend"

// TODO: check origin of properties used in bindings

Rectangle {
    id: bottomBar
    height: visible ? units.gu(6) : 0
    color: theme.palette.normal.background
    enabled: visible

    property FolderListModel folderModel
    property var selectionManager: folderModel.model.selectionObject
    property var fileOperationDialog

    property bool __actionsEnabled: (selectionManager.counter > 0) || (folderSelectorMode && folderModel.model.isWritable)
    property bool __actionsVisible: selectionMode

    ActionList {
        id: selectionActions

        Action {
            text: i18n.tr("Cut")
            iconName: "edit-cut"
            enabled: __actionsEnabled
            visible: __actionsVisible && folderModel.model.isWritable
            onTriggered: {
                var selectedAbsPaths = selectionManager.selectedAbsFilePaths();
                folderModel.model.cutPaths(selectedAbsPaths)
                selectionManager.clear()
                fileSelectorMode = false
                fileSelector.fileSelectorComponent = null
            }
        }

        Action {
            text: i18n.tr("Copy")
            iconName: "edit-copy"
            enabled: __actionsEnabled
            visible: __actionsVisible
            onTriggered: {
                var selectedAbsPaths = selectionManager.selectedAbsFilePaths();
                folderModel.model.copyPaths(selectedAbsPaths)
                selectionManager.clear()
                fileSelectorMode = false
                fileSelector.fileSelectorComponent = null
            }
        }


        Action {
            text: i18n.tr("Delete")
            iconName: "edit-delete"
            enabled: __actionsEnabled
            visible: __actionsVisible && folderModel.model.isWritable
            onTriggered: {
                var selectedAbsPaths = selectionManager.selectedAbsFilePaths();

                var props = {
                    "paths" : selectedAbsPaths,
                    "folderModel": folderModel.model,
                    "fileOperationDialog": fileOperationDialog
                }

                var dialog = PopupUtils.open(Qt.resolvedUrl("../dialogs/ConfirmMultipleDeleteDialog.qml"), mainView, props)

                dialog.accepted.connect(function() {
                    selectionManager.clear()
                    fileSelectorMode = false
                    fileSelector.fileSelectorComponent = null
                })
            }
        }
    }

    Flow {
        id: bottomBarButtons
        anchors.centerIn: parent
        spacing: units.gu(2)
        Repeater {
            id: repeater
            model: selectionActions.actions
            delegate: Button {
                action: modelData
                width: Math.min(bottomBar.width, units.gu(80)) / repeater.count - bottomBarButtons.spacing
                height: units.gu(5)
                anchors.topMargin: units.gu(1)
                color: theme.palette.normal.foreground
            }
        }
    }
}