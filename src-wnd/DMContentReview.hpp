/* DarkMark (C) 2019-2020 Stephane Charette <stephanecharette@gmail.com>
 * $Id$
 */

#pragma once

#include "DarkMark.hpp"


namespace dm
{
	class DMContentReview : public ThreadWithProgressWindow
	{
		public:

			DMContentReview(dm::DMContent & c);

			virtual ~DMContentReview();

			virtual void run();

			DMContent & content;
	};
}
