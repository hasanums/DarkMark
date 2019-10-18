/* DarkMark (C) 2019 Stephane Charette <stephanecharette@gmail.com>
 * $Id$
 */

#pragma once

#include <ctime>
#include <fstream>
#include <regex>
#include <thread>
#include <chrono>
#include <DarkHelp.hpp>

#include <JuceHeader.h>


// forward declare a few classes
namespace dm
{
	class Cfg;
	class DMWnd;
	class Mark;
	class DMCorner;
	class DMCanvas;
	class Notebook;
	class DMContent;
	class DMStatsWnd;
	class DMAboutWnd;
	class DarknetWnd;
	class StartupWnd;
	class StartupCanvas;
	class ProjectInfo;
	class CrosshairComponent;
	class DarkMarkApplication;

	typedef std::vector<std::string> VStr;
	typedef std::set<std::string> SStr;
}

#include "Log.hpp"
#include "Cfg.hpp"
#include "Bitmaps.hpp"
#include "Mark.hpp"
#include "Tools.hpp"
#include "CrosshairComponent.hpp"
#include "ProjectInfo.hpp"
#include "Notebook.hpp"
#include "DMCorner.hpp"
#include "DMCanvas.hpp"
#include "DMContent.hpp"
#include "DMStatsWnd.hpp"
#include "DMAboutWnd.hpp"
#include "DarknetWnd.hpp"
#include "StartupWnd.hpp"
#include "StartupCanvas.hpp"
#include "DMContentImageFilenameSort.hpp"
#include "DMContentStatistics.hpp"
#include "DMWnd.hpp"
#include "DarkMarkApp.hpp"
