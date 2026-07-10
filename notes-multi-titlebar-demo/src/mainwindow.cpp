#include "mainwindow.h"

#include "captionbuttonstrip.h"
#include "sidebartogglebutton.h"

#include <QAbstractItemView>
#include <QEasingCurve>
#include <QEvent>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QPushButton>
#include <QResizeEvent>
#include <QSplitter>
#include <QTextEdit>
#include <QVariantAnimation>
#include <QVBoxLayout>

#include <QWKWidgets/widgetwindowagent.h>

#include <algorithm>

namespace {

    constexpr int kTitleBarHeight = 52;
    constexpr int kCaptionButtonsWidth = 46 * 3;

    constexpr int kDefaultFoldersWidth = 230;
    constexpr int kDefaultListWidth = 330;
    constexpr int kCollapsedFoldersWidth = 0;
    constexpr int kMinimumRememberedFoldersWidth = 170;

    QPushButton *makeToolbarButton(const QString &text, QWidget *parent)
    {
        auto *button = new QPushButton(text, parent);
        button->setObjectName(QStringLiteral("toolbarButton"));
        button->setCursor(Qt::ArrowCursor);
        button->setFocusPolicy(Qt::NoFocus);
        return button;
    }

    QLabel *makeTitleLabel(const QString &text, const QString &objectName, QWidget *parent)
    {
        auto *label = new QLabel(text, parent);
        label->setObjectName(objectName);
        return label;
    }

    QString buildStyleSheet()
    {
        /*
            One inline light-mode stylesheet.

             The goal is not a pixel clone of Apple Notes, but the same architecture:
             soft light background, separate muted sidebar/list panes, strong editor
             canvas, subtle dividers, compact titlebar buttons, and yellow selection.
         */
        return QStringLiteral(R"(
QMainWindow {
    background: #fbfaf7;
}

QWidget#rootSurface {
    background: #fbfaf7;
}

QSplitter#notesSplitter {
    background: transparent;
    border: none;
}

QSplitter#notesSplitter::handle {
    background: rgba(0, 0, 0, 0.10);
    border: none;
    width: 1px;
}

QSplitter#notesSplitter::handle:hover {
    background: rgba(0, 0, 0, 0.18);
}

QWidget#foldersPane {
    background: #efede9;
    border: none;
}

QWidget#notesListPane {
    background: #f7f5f1;
    border: none;
}

QWidget#editorPane {
    background: #fffefa;
    border: none;
}

QWidget#foldersTitleBar,
QWidget#listTitleBar,
QWidget#editorTitleBar {
    background: rgba(255, 255, 255, 0.34);
    border: none;
}

QLabel#paneTitle {
    color: #2f3034;
    font-size: 14px;
    font-weight: 700;
}

QLabel#sectionLabel {
    color: #77777c;
    font-size: 11px;
    font-weight: 700;
    letter-spacing: 0.5px;
}

QPushButton#toolbarButton,
QPushButton#newFolderButton,
QPushButton#newNoteButton {
    color: #35363a;
    background: rgba(255, 255, 255, 0.55);
    border: 1px solid rgba(0, 0, 0, 0.055);
    border-radius: 8px;
    padding: 6px 10px;
    font-size: 12px;
    font-weight: 600;
}

QPushButton#toolbarButton:hover,
QPushButton#newFolderButton:hover,
QPushButton#newNoteButton:hover {
    background: rgba(255, 255, 255, 0.92);
}

QPushButton#toolbarButton:pressed,
QPushButton#newFolderButton:pressed,
QPushButton#newNoteButton:pressed {
    background: rgba(236, 235, 231, 1.0);
}

QLineEdit#searchEdit {
    color: #303136;
    background: rgba(255, 255, 255, 0.72);
    border: 1px solid rgba(0, 0, 0, 0.06);
    border-radius: 10px;
    padding: 7px 10px;
    font-size: 12px;
    selection-background-color: #ffe08a;
}

QListWidget#foldersList {
    background: transparent;
    border: none;
    outline: none;
    padding: 8px 8px 12px 8px;
}

QListWidget#foldersList::item {
    color: #34353a;
    background: transparent;
    border-radius: 8px;
    padding: 8px 10px;
    margin: 1px 0px;
}

QListWidget#foldersList::item:selected {
    color: #1e1f23;
    background: rgba(0, 0, 0, 0.075);
}

QListWidget#notesList {
    background: transparent;
    border: none;
    outline: none;
    padding: 8px 8px 12px 8px;
}

QListWidget#notesList::item {
    color: #3a3b40;
    background: transparent;
    border-radius: 12px;
    padding: 12px 12px;
    margin: 2px 0px;
}

QListWidget#notesList::item:selected {
    color: #1d1e22;
    background: #ffe28a;
}

QLabel#editorTitle {
    color: #17181c;
    font-size: 30px;
    font-weight: 700;
}

QLabel#editorMeta {
    color: #7c7c82;
    font-size: 12px;
}

QTextEdit#editor {
    color: #24252a;
    background: transparent;
    border: none;
    font-size: 15px;
    padding: 0px;
}

QFrame#thinDivider {
    background: rgba(0, 0, 0, 0.08);
    min-height: 1px;
    max-height: 1px;
    border: none;
}

CaptionButtonStrip#captionButtonStrip {
    background: transparent;
    border: none;
}
)");
    }

} // namespace

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    /*
        Do this before building the UI.
        QWindowKit's window agent adjusts native window behavior and internal
        frame calculations.
    */
    setAttribute(Qt::WA_DontCreateNativeAncestors);

    installWindowAgent();

    m_root = new QWidget(this);
    m_root->setObjectName(QStringLiteral("rootSurface"));

    auto *rootLayout = new QHBoxLayout(m_root);
    rootLayout->setContentsMargins(0, 0, 0, 0);
    rootLayout->setSpacing(0);

    /*
        Three-pane Apple Notes-style layout:
            0. Folders/accounts sidebar
            1. Notes list / previews
            2. Editor / selected note content

         The splitter begins at y = 0. There is no full-width overlay windowbar,
         so the top portions of the splitter handles remain draggable.
     */
    m_splitter = new QSplitter(Qt::Horizontal, m_root);
    m_splitter->setObjectName(QStringLiteral("notesSplitter"));
    m_splitter->setChildrenCollapsible(false);
    m_splitter->setOpaqueResize(true);
    m_splitter->setHandleWidth(1);

    m_foldersPane = createFoldersPane();
    m_notesListPane = createNotesListPane();
    m_editorPane = createEditorPane();

    m_splitter->addWidget(m_foldersPane);
    m_splitter->addWidget(m_notesListPane);
    m_splitter->addWidget(m_editorPane);

    m_splitter->setCollapsible(0, true);
    m_splitter->setCollapsible(1, false);
    m_splitter->setCollapsible(2, false);

    m_splitter->setStretchFactor(0, 0);
    m_splitter->setStretchFactor(1, 0);
    m_splitter->setStretchFactor(2, 1);

    rootLayout->addWidget(m_splitter);
    setCentralWidget(m_root);

    /*
        Fixed Windows caption buttons.

         They are not inside any pane titlebar because minimize/maximize/close
         belong to the top-level native window, not to a particular Notes pane.
     */
    m_captionButtons = new CaptionButtonStrip(this);
    m_captionButtons->show();

    connect(m_captionButtons->minimizeButton(), &QPushButton::clicked,
            this, &QWidget::showMinimized);

    connect(m_captionButtons->maximizeButton(), &QPushButton::clicked, this, [this]() {
        if (isMaximized()) {
            showNormal();
        } else {
            showMaximized();
        }
    });

    connect(m_captionButtons->closeButton(), &QPushButton::clicked,
            this, &QWidget::close);

    /*
        Sidebar collapse animation.

         We animate the first splitter section width directly. This keeps normal
         splitter behavior when the user drags manually, but gives the toggle
         button a smooth macOS-like collapse/expand feel.
     */
    m_foldersAnimation = new QVariantAnimation(this);
    m_foldersAnimation->setDuration(220);
    m_foldersAnimation->setEasingCurve(QEasingCurve::InOutCubic);

    connect(m_foldersAnimation, &QVariantAnimation::valueChanged, this,
            [this](const QVariant &value) {
                setFoldersPaneWidth(value.toInt());
            });

    connect(m_sidebarToggleButton, &QPushButton::clicked,
            this, &MainWindow::toggleFoldersPane);

    connect(m_splitter, &QSplitter::splitterMoved, this,
            [this](int, int) {
                const int width = foldersPaneWidth();

                if (width >= kMinimumRememberedFoldersWidth) {
                    m_lastExpandedFoldersWidth = width;
                }

                if (m_sidebarToggleButton) {
                    m_sidebarToggleButton->setCollapsed(width <= 4);
                }
            });

    populateNotes();
    registerTitleBarsAndHitTestWidgets();
    applyInlineStyleSheet();

    setWindowTitle(QStringLiteral("Notes"));
    resize(1280, 820);

    m_splitter->setSizes({
        kDefaultFoldersWidth,
        kDefaultListWidth,
        std::max(600, width() - kDefaultFoldersWidth - kDefaultListWidth)
    });

    selectNote(0);
    layoutOverlayChrome();
    syncCaptionButtonState();
}

void MainWindow::resizeEvent(QResizeEvent *event)
{
    QMainWindow::resizeEvent(event);
    layoutOverlayChrome();
}

void MainWindow::changeEvent(QEvent *event)
{
    QMainWindow::changeEvent(event);

    if (event->type() == QEvent::WindowStateChange) {
        syncCaptionButtonState();
        layoutOverlayChrome();
    }
}

QWidget *MainWindow::createFoldersPane()
{
    auto *pane = new QWidget;
    pane->setObjectName(QStringLiteral("foldersPane"));

    auto *outer = new QVBoxLayout(pane);
    outer->setContentsMargins(0, 0, 0, 0);
    outer->setSpacing(0);

    m_foldersTitleBar = new QWidget(pane);
    m_foldersTitleBar->setObjectName(QStringLiteral("foldersTitleBar"));
    m_foldersTitleBar->setFixedHeight(kTitleBarHeight);

    auto *titleLayout = new QHBoxLayout(m_foldersTitleBar);
    titleLayout->setContentsMargins(14, 10, 12, 10);
    titleLayout->setSpacing(8);

    auto *title = makeTitleLabel(QStringLiteral("Folders"), QStringLiteral("paneTitle"), m_foldersTitleBar);

    m_newFolderButton = new QPushButton(QStringLiteral("+"), m_foldersTitleBar);
    m_newFolderButton->setObjectName(QStringLiteral("newFolderButton"));
    m_newFolderButton->setCursor(Qt::ArrowCursor);
    m_newFolderButton->setFocusPolicy(Qt::NoFocus);
    m_newFolderButton->setFixedWidth(34);

    titleLayout->addWidget(title);
    titleLayout->addStretch(1);
    titleLayout->addWidget(m_newFolderButton);

    outer->addWidget(m_foldersTitleBar);

    auto *icloudLabel = makeTitleLabel(QStringLiteral("ICLOUD"), QStringLiteral("sectionLabel"), pane);

    auto *sectionWrapper = new QWidget(pane);
    auto *sectionLayout = new QHBoxLayout(sectionWrapper);
    sectionLayout->setContentsMargins(16, 8, 16, 0);
    sectionLayout->addWidget(icloudLabel);

    outer->addWidget(sectionWrapper);

    m_foldersList = new QListWidget(pane);
    m_foldersList->setObjectName(QStringLiteral("foldersList"));
    m_foldersList->setSelectionMode(QAbstractItemView::SingleSelection);
    m_foldersList->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    m_foldersList->addItem(QStringLiteral("All iCloud        42"));
    m_foldersList->addItem(QStringLiteral("Notes             29"));
    m_foldersList->addItem(QStringLiteral("Pinned             6"));
    m_foldersList->addItem(QStringLiteral("Work               8"));
    m_foldersList->addItem(QStringLiteral("Personal          11"));
    m_foldersList->addItem(QStringLiteral("Recently Deleted   2"));
    m_foldersList->setCurrentRow(0);

    outer->addWidget(m_foldersList, 1);

    return pane;
}

QWidget *MainWindow::createNotesListPane()
{
    auto *pane = new QWidget;
    pane->setObjectName(QStringLiteral("notesListPane"));

    auto *outer = new QVBoxLayout(pane);
    outer->setContentsMargins(0, 0, 0, 0);
    outer->setSpacing(0);

    m_listTitleBar = new QWidget(pane);
    m_listTitleBar->setObjectName(QStringLiteral("listTitleBar"));
    m_listTitleBar->setFixedHeight(kTitleBarHeight);

    auto *titleLayout = new QHBoxLayout(m_listTitleBar);
    titleLayout->setContentsMargins(12, 10, 12, 10);
    titleLayout->setSpacing(8);

    m_sidebarToggleButton = new SidebarToggleButton(m_listTitleBar);

    m_searchEdit = new QLineEdit(m_listTitleBar);
    m_searchEdit->setObjectName(QStringLiteral("searchEdit"));
    m_searchEdit->setPlaceholderText(QStringLiteral("Search"));
    m_searchEdit->setClearButtonEnabled(true);

    m_newNoteButton = new QPushButton(QStringLiteral("✎"), m_listTitleBar);
    m_newNoteButton->setObjectName(QStringLiteral("newNoteButton"));
    m_newNoteButton->setCursor(Qt::ArrowCursor);
    m_newNoteButton->setFocusPolicy(Qt::NoFocus);
    m_newNoteButton->setFixedWidth(36);

    titleLayout->addWidget(m_sidebarToggleButton);
    titleLayout->addWidget(m_searchEdit, 1);
    titleLayout->addWidget(m_newNoteButton);

    outer->addWidget(m_listTitleBar);

    auto *divider = new QFrame(pane);
    divider->setObjectName(QStringLiteral("thinDivider"));
    outer->addWidget(divider);

    m_notesList = new QListWidget(pane);
    m_notesList->setObjectName(QStringLiteral("notesList"));
    m_notesList->setSelectionMode(QAbstractItemView::SingleSelection);
    m_notesList->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    connect(m_notesList, &QListWidget::currentRowChanged,
            this, &MainWindow::selectNote);

    outer->addWidget(m_notesList, 1);

    return pane;
}

QWidget *MainWindow::createEditorPane()
{
    auto *pane = new QWidget;
    pane->setObjectName(QStringLiteral("editorPane"));

    auto *outer = new QVBoxLayout(pane);
    outer->setContentsMargins(0, 0, 0, 0);
    outer->setSpacing(0);

    m_editorTitleBar = new QWidget(pane);
    m_editorTitleBar->setObjectName(QStringLiteral("editorTitleBar"));
    m_editorTitleBar->setFixedHeight(kTitleBarHeight);

    auto *titleLayout = new QHBoxLayout(m_editorTitleBar);

    /*
        Reserve space for the fixed Windows caption buttons on the right.
        Without this margin, toolbar buttons may visually go under minimize /
        maximize / close.
    */
    titleLayout->setContentsMargins(14, 10, 14 + kCaptionButtonsWidth, 10);
    titleLayout->setSpacing(8);

    auto *scopeLabel = makeTitleLabel(QStringLiteral("All iCloud"), QStringLiteral("paneTitle"), m_editorTitleBar);

    m_galleryButton = makeToolbarButton(QStringLiteral("Gallery"), m_editorTitleBar);
    m_checklistButton = makeToolbarButton(QStringLiteral("Checklist"), m_editorTitleBar);
    m_tableButton = makeToolbarButton(QStringLiteral("Table"), m_editorTitleBar);
    m_shareButton = makeToolbarButton(QStringLiteral("Share"), m_editorTitleBar);
    m_moreButton = makeToolbarButton(QStringLiteral("More"), m_editorTitleBar);

    titleLayout->addWidget(scopeLabel);
    titleLayout->addStretch(1);
    titleLayout->addWidget(m_galleryButton);
    titleLayout->addWidget(m_checklistButton);
    titleLayout->addWidget(m_tableButton);
    titleLayout->addWidget(m_shareButton);
    titleLayout->addWidget(m_moreButton);

    outer->addWidget(m_editorTitleBar);

    auto *divider = new QFrame(pane);
    divider->setObjectName(QStringLiteral("thinDivider"));
    outer->addWidget(divider);

    auto *body = new QWidget(pane);
    auto *bodyLayout = new QVBoxLayout(body);
    bodyLayout->setContentsMargins(34, 26, 38, 28);
    bodyLayout->setSpacing(10);

    m_editorTitleLabel = new QLabel(body);
    m_editorTitleLabel->setObjectName(QStringLiteral("editorTitle"));

    m_editorMetaLabel = new QLabel(body);
    m_editorMetaLabel->setObjectName(QStringLiteral("editorMeta"));

    m_editor = new QTextEdit(body);
    m_editor->setObjectName(QStringLiteral("editor"));
    m_editor->setFrameStyle(QFrame::NoFrame);

    bodyLayout->addWidget(m_editorTitleLabel);
    bodyLayout->addWidget(m_editorMetaLabel);
    bodyLayout->addSpacing(8);
    bodyLayout->addWidget(m_editor, 1);

    outer->addWidget(body, 1);

    return pane;
}

void MainWindow::installWindowAgent()
{
    m_windowAgent = new QWK::WidgetWindowAgent(this);
    m_windowAgent->setup(this);
}

void MainWindow::registerTitleBarsAndHitTestWidgets()
{
    /*
        This requires your patched QWindowKit.

         Three independent draggable titlebars:
             - folders titlebar
             - notes list titlebar
             - editor titlebar

          QWindowKit's Windows backend should treat the union of these regions as
          HTCAPTION, except for registered interactive controls and system buttons.
      */
    m_windowAgent->addTitleBar(m_foldersTitleBar);
    m_windowAgent->addTitleBar(m_listTitleBar);
    m_windowAgent->addTitleBar(m_editorTitleBar);

    /*
        Any clickable child inside a registered titlebar must be marked
        hit-test-visible. Otherwise QWindowKit will classify that child area as
        draggable titlebar and Windows will receive HTCAPTION instead of Qt
        receiving the mouse click.
    */
    m_windowAgent->setHitTestVisible(m_foldersTitleBar, m_newFolderButton, true);
    m_windowAgent->setHitTestVisible(m_listTitleBar, m_sidebarToggleButton, true);
    m_windowAgent->setHitTestVisible(m_listTitleBar, m_searchEdit, true);
    m_windowAgent->setHitTestVisible(m_listTitleBar, m_newNoteButton, true);

    m_windowAgent->setHitTestVisible(m_editorTitleBar, m_galleryButton, true);
    m_windowAgent->setHitTestVisible(m_editorTitleBar, m_checklistButton, true);
    m_windowAgent->setHitTestVisible(m_editorTitleBar, m_tableButton, true);
    m_windowAgent->setHitTestVisible(m_editorTitleBar, m_shareButton, true);
    m_windowAgent->setHitTestVisible(m_editorTitleBar, m_moreButton, true);

    /*
        System buttons are global per window. Do not create one set per pane.

         QWindowKit maps these to native HTMINBUTTON / HTMAXBUTTON / HTCLOSE
         equivalents on Windows, preserving native caption behavior such as
         Windows 11 Snap Layout on the maximize button.
     */
    m_windowAgent->setSystemButton(QWK::WindowAgentBase::Minimize,
                                   m_captionButtons->minimizeButton());
    m_windowAgent->setSystemButton(QWK::WindowAgentBase::Maximize,
                                   m_captionButtons->maximizeButton());
    m_windowAgent->setSystemButton(QWK::WindowAgentBase::Close,
                                   m_captionButtons->closeButton());
}

void MainWindow::applyInlineStyleSheet()
{
    setStyleSheet(buildStyleSheet());
}

void MainWindow::layoutOverlayChrome()
{
    if (!m_captionButtons) {
        return;
    }

    m_captionButtons->setGeometry(width() - m_captionButtons->preferredWidth(),
                                  0,
                                  m_captionButtons->preferredWidth(),
                                  m_captionButtons->preferredHeight());
    m_captionButtons->raise();
}

void MainWindow::syncCaptionButtonState()
{
    if (m_captionButtons) {
        m_captionButtons->syncWindowState(isMaximized());
    }
}

void MainWindow::populateNotes()
{
    m_noteTitles = {
        QStringLiteral("Project Status"),
        QStringLiteral("Monday Morning Meeting"),
        QStringLiteral("QWindowKit Patch Notes"),
        QStringLiteral("Travel Ideas"),
        QStringLiteral("Grocery List"),
        QStringLiteral("Reading Queue"),
        QStringLiteral("UI Details")
    };

    m_noteDates = {
        QStringLiteral("Today at 9:42 AM"),
        QStringLiteral("Yesterday at 2:15 PM"),
        QStringLiteral("Yesterday at 11:30 AM"),
        QStringLiteral("Previous 7 Days"),
        QStringLiteral("Previous 7 Days"),
        QStringLiteral("Previous 30 Days"),
        QStringLiteral("Previous 30 Days")
    };

    m_noteSubtitles = {
        QStringLiteral("Multi-titlebar support and demo verification."),
        QStringLiteral("Agenda, budget check-in, action items."),
        QStringLiteral("Keep setTitleBar compatible; add additive APIs."),
        QStringLiteral("Seattle in October, rain jacket, camera list."),
        QStringLiteral("Eggs, milk, coffee filters, fruit."),
        QStringLiteral("DWM custom frame docs and Qt native events."),
        QStringLiteral("Pane-local titlebars, exposed splitter handles.")
    };

    m_noteBodies = {
        QStringLiteral(
            "Deliverable A\n"
            "  • QWindowKit should support multiple draggable titlebar regions.\n"
            "  • setTitleBar() remains the legacy replacement API.\n"
            "  • addTitleBar() appends additional draggable regions.\n"
            "  • System buttons remain globally unique per top-level window.\n\n"
            "Deliverable B\n"
            "  • The demo should use three panes like Apple Notes.\n"
            "  • Left pane: folders and accounts.\n"
            "  • Middle pane: note previews.\n"
            "  • Right pane: selected note editor.\n\n"
            "Result\n"
            "  • Splitter handles are never covered by a full-width overlay titlebar.\n"
            "  • Each pane titlebar contributes to the native draggable region.\n"
            "  • Caption buttons stay fixed at the top-right corner."),

        QStringLiteral(
            "Agenda\n"
            "  • Review QWindowKit multi-titlebar patch.\n"
            "  • Verify Windows Snap Layout still works.\n"
            "  • Confirm hit-test-visible widgets inside all three titlebars.\n\n"
            "Budget check-in\n"
            "  • Keep demo resource-free.\n"
            "  • Use inline QSS.\n"
            "  • Avoid custom images and qrc files."),

        QStringLiteral(
            "Core changes\n"
            "  • Replace single m_titleBar pointer with QList<QPointer<QObject>>.\n"
            "  • Add titleBars(), addTitleBar(), removeTitleBar(), clearTitleBars().\n"
            "  • Make isInTitleBarDraggableArea() iterate over all registered bars.\n\n"
            "Important\n"
            "  • Do not change WM_NCCALCSIZE.\n"
            "  • Do not add extra winId() calls.\n"
            "  • Keep system-button hit-test before titlebar hit-test."),

        QStringLiteral(
            "Seattle trip\n"
            "  • Light rain jacket.\n"
            "  • 35mm lens.\n"
            "  • Coffee shops near Capitol Hill.\n"
            "  • Ferry if weather is good."),

        QStringLiteral(
            "Groceries\n"
            "  □ Eggs\n"
            "  □ Milk\n"
            "  □ Coffee filters\n"
            "  □ Bananas\n"
            "  □ Greek yogurt\n"
            "  □ Rice"),

        QStringLiteral(
            "Reading queue\n"
            "  • Qt QWidget nativeEvent documentation.\n"
            "  • Microsoft WM_NCHITTEST documentation.\n"
            "  • Microsoft WM_NCCALCSIZE documentation.\n"
            "  • QWindowKit Win32WindowContext source."),

        QStringLiteral(
            "UI notes\n"
            "  • Folders pane should feel muted and structural.\n"
            "  • Notes list should feel slightly brighter.\n"
            "  • Editor should be almost paper-white.\n"
            "  • Selection should use a warm Notes-like yellow.\n"
            "  • Toolbar controls should stay compact and soft.")
    };

    for (int i = 0; i < m_noteTitles.size(); ++i) {
        const QString text = QStringLiteral("%1\n%2\n%3")
        .arg(m_noteTitles.at(i),
             m_noteDates.at(i),
             m_noteSubtitles.at(i));

        auto *item = new QListWidgetItem(text);
        item->setSizeHint(QSize(260, 78));
        m_notesList->addItem(item);
    }

    m_notesList->setCurrentRow(0);
}

void MainWindow::selectNote(int index)
{
    if (index < 0 || index >= m_noteTitles.size()) {
        return;
    }

    m_editorTitleLabel->setText(m_noteTitles.at(index));
    m_editorMetaLabel->setText(m_noteDates.at(index) + QStringLiteral("  •  iCloud"));
    m_editor->setPlainText(m_noteBodies.at(index));
}

void MainWindow::toggleFoldersPane()
{
    const bool collapsed = foldersPaneWidth() <= 4;

    const int targetWidth = collapsed ? expandedFoldersPaneWidth()
                                      : kCollapsedFoldersWidth;

    if (m_sidebarToggleButton) {
        m_sidebarToggleButton->setCollapsed(!collapsed);
    }

    animateFoldersPaneTo(targetWidth);
}

void MainWindow::animateFoldersPaneTo(int targetWidth)
{
    const int start = foldersPaneWidth();
    const int maxLeft = std::max(0, availableSplitterWidth() - 520);
    const int end = std::clamp(targetWidth, 0, maxLeft);

    if (start == end) {
        return;
    }

    m_foldersAnimation->stop();
    m_foldersAnimation->setStartValue(start);
    m_foldersAnimation->setEndValue(end);
    m_foldersAnimation->start();
}

void MainWindow::setFoldersPaneWidth(int width)
{
    if (!m_splitter) {
        return;
    }

    const QList<int> oldSizes = m_splitter->sizes();

    int middle = oldSizes.size() > 1 ? oldSizes.at(1) : kDefaultListWidth;
    int right = oldSizes.size() > 2 ? oldSizes.at(2) : 700;

    const int total = availableSplitterWidth();
    const int left = std::clamp(width, 0, std::max(0, total - 520));

    const int remaining = std::max(0, total - left);

    if (middle < 250) {
        middle = 250;
    }

    if (middle > remaining - 320) {
        middle = std::max(250, remaining / 3);
    }

    right = std::max(320, remaining - middle);

    m_splitter->setSizes({left, middle, right});

    if (left >= kMinimumRememberedFoldersWidth) {
        m_lastExpandedFoldersWidth = left;
    }
}

int MainWindow::foldersPaneWidth() const
{
    if (!m_splitter) {
        return 0;
    }

    const QList<int> sizes = m_splitter->sizes();

    if (sizes.isEmpty()) {
        return 0;
    }

    return sizes.first();
}

int MainWindow::expandedFoldersPaneWidth() const
{
    return std::max(kMinimumRememberedFoldersWidth, m_lastExpandedFoldersWidth);
}

int MainWindow::availableSplitterWidth() const
{
    if (!m_splitter) {
        return 0;
    }

    return std::max(0, m_splitter->width());
}