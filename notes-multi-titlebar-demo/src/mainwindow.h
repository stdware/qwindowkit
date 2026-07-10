#pragma once

#include <QMainWindow>
#include <QStringList>

class QLabel;
class QLineEdit;
class QListWidget;
class QPushButton;
class QSplitter;
class QTextEdit;
class QVariantAnimation;
class QWidget;

class CaptionButtonStrip;
class SidebarToggleButton;

namespace QWK {
    class WidgetWindowAgent;
}

class MainWindow final : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override = default;

protected:
    void resizeEvent(QResizeEvent *event) override;
    void changeEvent(QEvent *event) override;

private:
    QWidget *createFoldersPane();
    QWidget *createNotesListPane();
    QWidget *createEditorPane();

    void installWindowAgent();
    void registerTitleBarsAndHitTestWidgets();

    void applyInlineStyleSheet();
    void layoutOverlayChrome();
    void syncCaptionButtonState();

    void populateNotes();
    void selectNote(int index);

    void toggleFoldersPane();
    void animateFoldersPaneTo(int targetWidth);
    void setFoldersPaneWidth(int width);
    int foldersPaneWidth() const;
    int expandedFoldersPaneWidth() const;
    int availableSplitterWidth() const;

private:
    QWK::WidgetWindowAgent *m_windowAgent = nullptr;

    QWidget *m_root = nullptr;
    QSplitter *m_splitter = nullptr;

    QWidget *m_foldersPane = nullptr;
    QWidget *m_notesListPane = nullptr;
    QWidget *m_editorPane = nullptr;

    QWidget *m_foldersTitleBar = nullptr;
    QWidget *m_listTitleBar = nullptr;
    QWidget *m_editorTitleBar = nullptr;

    SidebarToggleButton *m_sidebarToggleButton = nullptr;

    QPushButton *m_newFolderButton = nullptr;
    QPushButton *m_newNoteButton = nullptr;
    QPushButton *m_galleryButton = nullptr;
    QPushButton *m_checklistButton = nullptr;
    QPushButton *m_tableButton = nullptr;
    QPushButton *m_shareButton = nullptr;
    QPushButton *m_moreButton = nullptr;

    QLineEdit *m_searchEdit = nullptr;

    CaptionButtonStrip *m_captionButtons = nullptr;

    QListWidget *m_foldersList = nullptr;
    QListWidget *m_notesList = nullptr;

    QLabel *m_editorTitleLabel = nullptr;
    QLabel *m_editorMetaLabel = nullptr;
    QTextEdit *m_editor = nullptr;

    QVariantAnimation *m_foldersAnimation = nullptr;

    int m_lastExpandedFoldersWidth = 230;

    QStringList m_noteTitles;
    QStringList m_noteSubtitles;
    QStringList m_noteDates;
    QStringList m_noteBodies;
};