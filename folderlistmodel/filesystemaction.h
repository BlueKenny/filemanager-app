/**************************************************************************
 *
 * Copyright 2013 Canonical Ltd.
 * Copyright 2013 Carlos J Mazieri <carlos.mazieri@gmail.com>
 *
 * You may use this file under the terms of the BSD license as follows:
 *
 * "Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *   * Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in
 *     the documentation and/or other materials provided with the
 *     distribution.
 *   * Neither the name of Nemo Mobile nor the names of its contributors
 *     may be used to endorse or promote products derived from this
 *     software without specific prior written permission.
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
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
 *
 * File: filesystemaction.h
 * Date: 3/13/2013
 */

#ifndef FILESYSTEMACTION_H
#define FILESYSTEMACTION_H


#include <QObject>
#include <QFileInfo>
#include <QVector>

/*!
 *  used to inform any view that an item has been updated under a copy process
 *  every amount of bytes saved it emits a signal with the current file information
 */
#define AMOUNT_COPIED_TO_REFRESH_ITEM_INFO  50000000

class DirModelMimeData;
class QFile;
class QTemporaryFile;

enum ClipboardOperation
{
    NoClipboard, ClipboardCopy, ClipboardCut
};

/*!
 * \brief The FileSystemAction class does file system operations copy/cut/paste/remove items
 *
 * Implementation:
 * --------------
 * Remove and Paste (from either Copy or Cut) operations are performed by creating a list of items and putting this list
 * inside a \ref Action data structure. Each item is an \ref ActionEntry, if this item is a directory this ActionEntry will
 * will be expanded to have the whole directory content recursively, so before performing an Action the whole list of items
 * are built.
 * After an item be performed (an \ref ActionEntry) the \ref endCurrentAction()  emits signals of:
 * \ref progress(), \ref added() and \ref removed() or \ref changed() for cases where an item is overwritten.
 * These signals are also emitted when processing a such number of files inside an entry, in the case an entry is
 * a directory, the define \ref STEP_FILES is used for that.
 *
 * It is a single thread processing, some slots are used to work a little and then they are scheduled to continue
 * working in the next main loop interaction, this flow is controlled by:
 *  \li \ref processAction()           -> starts an \ref Action
 *  \li \ref processActionEntry()      -> starts an \ref ActionEntry
 *  \li \ref endActionEntry()          -> ends an \ref ActionEntry
 *  \li \ref processCopyEntry()        -> starts an \ref copy from an \ref ActionEntry
 *  \li \ref processCopySingleFile()   -> perform single file copy, it may have many interactions if the file is big,
 *                                        each interaction it writes (4KB * STEP_FILES) and emit \ref progress() signal
 *                                        and schedules itself for next write or \ref processCopyEntry() if it has already
 *                                        finished.
 *
 * Behavior:
 * ---------
 * \li Paste operations are made as single move using QFile::rename()
 * \li After pasting from a Cut operation, if no other application has changed the clipboard,
 *     the destination becomes source in the clipboard as Copy for further paste operations
 * \li Pasting in the same place where Cut was made is not allowed, an \ref error() signal is emitted
 * \li Pasting in the same place where Copy was made causes an automatic rename to identify it as backuped item
 * \li Paste from Copy when the destination already exists: individual files are overwritten
 *     and both signals \ref added() and \ref removed() are emitted, directories are not touched.
 * \li Paste  from Cut when the destination already exists: existent items (files or directories) are removed first,
 *     directories are removed in a special way, they are first moved to a temporary area and then scheduled to be removed later
 *     by creating an  auxiliary Remove \ref Action, see \ref moveDirToTempAndRemoveItLater().
 */
class FileSystemAction : public QObject
{
    Q_OBJECT
public:  
    explicit FileSystemAction(QObject *parent = 0);
    ~FileSystemAction();

public:
    bool     isBusy() const;
    int      getProgressCounter() const;
    int      clipboardLocalUrlsConunter();

public slots:
    void     cancel();
    void     remove(const QStringList & filePaths);
    void     pathChanged(const QString& path);
    void     paste();
    void     cut(const QStringList&);
    void     copy(const QStringList&);

signals:
    void     error(const QString& errorTitle, const QString &errorMessage);
    void     removed(const QString& item);
    void     removed(const QFileInfo&);
    void     added(const QString& );
    void     added(const QFileInfo& );
    void     changed(const QFileInfo&);
    void     progress(int curItem, int totalItems, int percent);
    void     clipboardChanged();

private slots:
    void     processAction();
    void     processActionEntry();   
    void     processCopyEntry();
    bool     processCopySingleFile();
    void     clipboardHasChanged();

 private:
   enum ActionType
   {
       ActionRemove,
       ActionCopy,
       ActionMove,
       ActionHardMoveCopy,
       ActionHardMoveRemove
   };
   void     createAndProcessAction(ActionType actionType, const QStringList& paths,
                                   ClipboardOperation operation=NoClipboard);
   struct CopyFile
   {
     public:
       CopyFile() : bytesWritten(0), source(0), target(0),
                    isEntryItem(false) , amountSavedToRefresh(AMOUNT_COPIED_TO_REFRESH_ITEM_INFO)
       {}
       ~CopyFile() { clear(); }
       void clear();

       qint64            bytesWritten;           // set 0 when reach  bytesToNotify, notify progress
       QFile          *  source;
       QFile          *  target;
       QString           targetName;
       bool              isEntryItem;  //true when the file being copied is at toplevel of the copy/cut operation
       qint64            amountSavedToRefresh;
   };

   /*!
       An ActionEntry represents a high level item as a File or a Directory which an Action is required

       For directories \a reversedOrder keeps all children
    */
   struct ActionEntry
   {
     public:
       ActionEntry(): currStep(0),currItem(0),alreadyExists(false), newName(0), added(false) {}
       ~ActionEntry()
       {
           reversedOrder.clear();
           if (newName) { delete newName; }
       }
       QList<QFileInfo>   reversedOrder;   //!< last item must be the item from the list
       int                currStep;
       int                currItem;
       bool               alreadyExists;
       QString *          newName; //TODO:  allow to rename an existent file when it already exists.
                                   //       So far it is possible to backup items when copy/paste in the
                                   //       same place, in this case it is renamed to "<name> Copy (%d).termination"

       bool               added;   //!< signal added() already emitted for the current ActionEntry
   };

   struct Action
   {
    public:
       ~Action()           {qDeleteAll(entries); entries.clear(); copyFile.clear();}
       ActionType          type;
       QList<ActionEntry*> entries;
       int                 totalItems;
       int                 currItem;
       int                 baseOrigSize;
       QString             origPath;
       QString             targetPath;
       quint64             totalBytes;
       quint64             bytesWritten;
       int                 currEntryIndex;
       ActionEntry  *      currEntry;
       ClipboardOperation  operation;
       CopyFile            copyFile;
       bool                done;
       Action *            auxAction;
       bool                isAux;
       int                 steps;
   };

   QVector<Action*>        m_queuedActions;  //!< work always at item 0, after finishing taking item 0 out
   Action            *     m_curAction;
   bool                    m_cancelCurrentAction;
   bool                    m_busy; 
   QString                 m_path;
   DirModelMimeData  *     m_mimeData;
   QString                 m_errorTitle;
   QString                 m_errorMsg;
   bool                    m_clipboardModifiedByOther;

private:  
   Action * createAction(ActionType, int origBase = 0);
   void     addEntry(Action* action, const QString &pathname);
   void     removeEntry(ActionEntry *);   
   void     moveEntry(ActionEntry *entry);
   bool     moveUsingSameFileSystem(const QString& itemToMovePathname);
   QString  targetFom(const QString& origItem, const Action * const action);
   void     endCurrentAction();
   int      percentWorkDone();
   int      notifyProgress(int forcePercent = 0);
   void     endActionEntry();
   bool     copySymLink(const QString& target, const QFileInfo& orig);
   void     scheduleSlot(const char *slot);
   void     moveDirToTempAndRemoveItLater(const QString& dir);
   bool     makeBackupNameForCurrentItem(Action *action);
   void     storeOnClipboard(const QStringList &pathnames, ClipboardOperation op);
   bool     endCopySingleFile();
   bool     isThereDiskSpace(qint64 requiredSize);

#if defined(REGRESSION_TEST_FOLDERLISTMODEL) //used in Unit/Regression tests
   friend class TestDirModel;
#endif
};


#endif // FILESYSTEMACTION_H
