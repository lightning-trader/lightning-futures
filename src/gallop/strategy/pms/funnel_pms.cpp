#include "funnel_pms.h"
#include "data_types.hpp"

uint32_t funnel_pms::get_sell_volume(offset_type offset)
{
	auto theory_sell = static_cast<uint32_t>(std::round(_pos.get_short_position() * _beta + _once));
	if (_opt != TO_OPEN_TO_CLOSE && offset == OT_OPEN)
	{
		return theory_sell;
	}
	auto yestoday_long = _pos.yestoday_long.usable();
	if(yestoday_long > 0)
	{
		return theory_sell > yestoday_long ? yestoday_long: theory_sell;
	}
	auto today_long = _pos.today_long.usable();
	if (today_long > 0)
	{
		return theory_sell > today_long ? today_long : theory_sell;
	}
	return theory_sell;
}


uint32_t funnel_pms::get_buy_volume(offset_type offset)
{
	auto theory_buy = static_cast<uint32_t>(std::round(_pos.get_long_position() * _beta + _once));
	if (_opt != TO_OPEN_TO_CLOSE && offset == OT_OPEN)
	{
		return theory_buy;
	}
	auto yestoday_short = _pos.yestoday_short.usable();
	if (yestoday_short > 0)
	{
		return theory_buy > yestoday_short ? yestoday_short : theory_buy;
	}
	auto today_short = _pos.today_short.usable();
	if (today_short > 0)
	{
		return theory_buy > today_short ? today_short : theory_buy;
	}
	return theory_buy;
}
