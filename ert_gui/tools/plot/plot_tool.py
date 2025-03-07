from ert_gui.ertwidgets import resourceIcon
from ert_gui.tools import Tool
from ert_gui.tools.plot import PlotWindow


class PlotTool(Tool):
    def __init__(self, config_file):
        super().__init__("Create plot", "tools/plot", resourceIcon("timeline.svg"))
        self._config_file = config_file

    def trigger(self):
        plot_window = PlotWindow(self._config_file, self.parent())
        plot_window.show()
