from qtpy.QtCore import Qt
from qtpy.QtWidgets import (
    QToolButton,
    QTextEdit,
    QTabWidget,
    QWidget,
    QVBoxLayout,
    QHBoxLayout,
    QLabel,
    QPushButton,
    QSpinBox,
)

from ert_gui.ertwidgets import addHelpToWidget, showWaitCursorWhileWaiting
from ert_gui.ertwidgets.caselist import CaseList
from ert_gui.ertwidgets.caseselector import CaseSelector
from ert_gui.ertwidgets.checklist import CheckList
from ert_gui.ertwidgets.models.ertmodel import (
    initializeCurrentCaseFromScratch,
    initializeCurrentCaseFromExisting,
)
from ert_gui.ertwidgets.models.selectable_list_model import SelectableListModel
from ert_shared.libres_facade import LibresFacade
from res.enkf import EnkfVarType


def createCheckLists(ert):
    parameter_model = SelectableListModel([])

    def getParameterList():
        return ert.ensembleConfig().getKeylistFromVarType(EnkfVarType.PARAMETER)

    parameter_model.getList = getParameterList
    parameter_check_list = CheckList(
        parameter_model, "Parameters", "init/select_parameters"
    )
    parameter_check_list.setMaximumWidth(300)

    members_model = SelectableListModel([])

    def getMemberList():
        return [str(member) for member in range(ert.getEnsembleSize())]

    members_model.getList = getMemberList
    member_check_list = CheckList(members_model, "Members", "init/select_members")
    member_check_list.setMaximumWidth(150)
    return (
        createRow(parameter_check_list, member_check_list),
        parameter_model,
        members_model,
    )


def createRow(*widgets):
    row = QHBoxLayout()

    for widget in widgets:
        row.addWidget(widget)

    row.addStretch()
    return row


class CaseInitializationConfigurationPanel(QTabWidget):
    @showWaitCursorWhileWaiting
    def __init__(self, ert, notifier):
        self.ert = ert
        self.notifier = notifier
        QTabWidget.__init__(self)
        self.setWindowTitle("Case management")
        self.setMinimumWidth(600)

        self.addCreateNewCaseTab()
        self.addInitializeFromScratchTab()
        self.addInitializeFromExistingTab()
        self.addShowCaseInfo()

    def addCreateNewCaseTab(self):
        panel = QWidget()
        layout = QVBoxLayout()
        case_list = CaseList(LibresFacade(self.ert), self.notifier)
        case_list.setMaximumWidth(250)

        layout.addWidget(case_list)
        layout.addStretch()

        panel.setLayout(layout)

        self.addTab(panel, "Create new case")

    def addInitializeFromScratchTab(self):
        panel = QWidget()
        layout = QVBoxLayout()

        row1 = createRow(
            QLabel("Target case:"), CaseSelector(LibresFacade(self.ert), self.notifier)
        )
        layout.addLayout(row1)

        check_list_layout, parameter_model, members_model = createCheckLists(self.ert)
        layout.addLayout(check_list_layout)

        layout.addSpacing(10)

        initialize_button = QPushButton("Initialize")
        addHelpToWidget(initialize_button, "init/initialize_from_scratch")
        initialize_button.setMinimumWidth(75)
        initialize_button.setMaximumWidth(150)

        def initializeFromScratch():
            parameters = parameter_model.getSelectedItems()
            members = members_model.getSelectedItems()
            initializeCurrentCaseFromScratch(parameters, members, self.ert)

        initialize_button.clicked.connect(initializeFromScratch)
        layout.addWidget(initialize_button, 0, Qt.AlignCenter)

        layout.addSpacing(10)

        panel.setLayout(layout)
        self.addTab(panel, "Initialize from scratch")

    def addInitializeFromExistingTab(self):
        widget = QWidget()
        layout = QVBoxLayout()

        target_case = CaseSelector(LibresFacade(self.ert), self.notifier)
        row = createRow(QLabel("Target case:"), target_case)
        layout.addLayout(row)

        source_case = CaseSelector(
            LibresFacade(self.ert),
            self.notifier,
            update_ert=False,
            show_only_initialized=True,
            ignore_current=True,
        )
        row = createRow(QLabel("Source case:"), source_case)
        layout.addLayout(row)

        row, history_length_spinner = self.createTimeStepRow()
        layout.addLayout(row)

        layout.addSpacing(10)
        check_list_layout, parameter_model, members_model = createCheckLists(self.ert)
        layout.addLayout(check_list_layout)
        layout.addSpacing(10)

        initialize_button = QPushButton("Initialize")
        addHelpToWidget(initialize_button, "init/initialize_from_existing")
        initialize_button.setMinimumWidth(75)
        initialize_button.setMaximumWidth(150)

        def initializeFromExisting():
            source_case_name = str(source_case.currentText())
            target_case_name = str(target_case.currentText())
            report_step = history_length_spinner.value()
            parameters = parameter_model.getSelectedItems()
            members = members_model.getSelectedItems()
            initializeCurrentCaseFromExisting(
                source_case_name,
                target_case_name,
                report_step,
                parameters,
                members,
                self.ert,
            )

        initialize_button.clicked.connect(initializeFromExisting)
        layout.addWidget(initialize_button, 0, Qt.AlignCenter)

        layout.addSpacing(10)

        layout.addStretch()
        widget.setLayout(layout)
        self.addTab(widget, "Initialize from existing")

    def createTimeStepRow(self):
        history_length_spinner = QSpinBox()
        addHelpToWidget(history_length_spinner, "config/init/history_length")
        history_length_spinner.setMinimum(0)
        history_length_spinner.setMaximum(self.ert.getHistoryLength())

        initial = QToolButton()
        initial.setText("Initial")
        addHelpToWidget(initial, "config/init/history_length")

        def setToMin():
            history_length_spinner.setValue(0)

        initial.clicked.connect(setToMin)

        end_of_time = QToolButton()
        end_of_time.setText("End of time")
        addHelpToWidget(end_of_time, "config/init/history_length")

        def setToMax():
            history_length_spinner.setValue(self.ert.getHistoryLength())

        end_of_time.clicked.connect(setToMax)

        row = createRow(
            QLabel("Timestep:"), history_length_spinner, initial, end_of_time
        )

        return row, history_length_spinner

    def addShowCaseInfo(self):
        case_widget = QWidget()
        layout = QVBoxLayout()

        case_selector = CaseSelector(
            LibresFacade(self.ert),
            self.notifier,
            update_ert=False,
            help_link="init/selected_case_info",
        )
        row1 = createRow(QLabel("Select case:"), case_selector)

        layout.addLayout(row1)

        self._case_info_area = QTextEdit()
        self._case_info_area.setReadOnly(True)
        self._case_info_area.setMinimumHeight(300)

        row2 = createRow(QLabel("Case info:"), self._case_info_area)

        layout.addLayout(row2)

        case_widget.setLayout(layout)

        case_selector.currentIndexChanged[str].connect(self._showInfoForCase)
        self.notifier.ertChanged.connect(self._showInfoForCase)

        self.addTab(case_widget, "Case info")

        self._showInfoForCase()

    def _showInfoForCase(self, case_name=None):
        if case_name is None:
            case_name = self.ert.getEnkfFsManager().getCurrentFileSystem().getCaseName()

        states = list(self.ert.getEnkfFsManager().getStateMapForCase(case_name))

        html = "<table>"
        for index, value in enumerate(states):
            html += f"<tr><td width=30>{index:d}.</td><td>{value}</td></tr>"

        html += "</table>"

        self._case_info_area.setHtml(html)
