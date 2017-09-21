import QtQuick 2.4
import Ubuntu.Components 1.3
import Ubuntu.Components.Popups 1.3

import "../backend"

// TODO: check origin of properties used in bindings

Rectangle {
    id: bottomBar
    height: visible ? bottomBarButtons.height : 0
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
            text: i18n.tr("Select")
            iconName: "tick"
            enabled: __actionsEnabled
            visible: __actionsVisible && isContentHub     // 'isContentHub' property declared in root QML file
            onTriggered: {
                var selectedAbsUrls = []
                if (folderSelectorMode) {
                    selectedAbsUrls = [ folder ]
                } else {
                    var selectedAbsPaths = selectionManager.selectedAbsFilePaths();
                    // For now support only selection in filesystem
                    selectedAbsUrls = selectedAbsPaths.map(function(item) {
                        return "file://" + item;
                    });
                }
                console.log("FileSelector OK clicked, selected items: " + selectedAbsUrls)
                acceptFileSelector(selectedAbsUrls)
            }
        }
        Action {
            text: i18n.tr("Cut")
            iconName: "edit-cut"
            enabled: __actionsEnabled
            visible: __actionsVisible && folderModel.model.isWritable && !isContentHub    // 'isContentHub' property declared in root QML file
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
            visible: __actionsVisible && !isContentHub    // 'isContentHub' property declared in root QML file
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
            visible: __actionsVisible && folderModel.model.isWritable && !isContentHub    // 'isContentHub' property declared in root QML file
            onTriggered: {
                var selectedAbsPaths = selectionManager.selectedAbsFilePaths();

                var props = {
                    "paths" : selectedAbsPaths,
                    "folderModel": folderModel.model,
                    "fileOperationDialog": fileOperationDialog
                }

                PopupUtils.open(Qt.resolvedUrl("../dialogs/ConfirmMultipleDeleteDialog.qml"), mainView, props)
                selectionManager.clear()
                fileSelectorMode = false
                fileSelector.fileSelectorComponent = null
            }
        }
    }

    Flow {
        id: bottomBarButtons
        height: units.gu(7)
        spacing: units.gu(2)
        anchors.horizontalCenter: parent.horizontalCenter
        Repeater{
            model: selectionActions.actions
            delegate: Button {
                // property bool showText: false
                action: modelData
                width: bottomBar.width / 3 - bottomBarButtons.spacing
                height: units.gu(5)
                anchors.topMargin: units.gu(1)
                color: "white"
            }
        }
    }
}