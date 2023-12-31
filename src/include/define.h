﻿#pragma once
#include <stdio.h>
#include <string>
#include <set>
#include <map>
#include <any>
#include <array>
#include <vector>
#include <stdint.h>
#include <memory>
#include <functional>
#include <cstring>
#include <cmath>
#include <cstdarg>
#include <unordered_map>

#ifndef EXPORT_FLAG
#ifdef _MSC_VER
#	define EXPORT_FLAG __declspec(dllexport)
#else
#	define EXPORT_FLAG __attribute__((__visibility__("default")))
#endif
#endif

#ifndef PORTER_FLAG
#ifdef _MSC_VER
#	define PORTER_FLAG _cdecl
#else
#	define PORTER_FLAG 
#endif
#endif

struct code_t;

typedef uint8_t untid_t;

//日内时间（精度到毫秒）
typedef uint32_t daytm_t;

constexpr uint8_t MAX_UNITID = 0xFFU;

//#define MAX_UNITID 0xFFU 

typedef uint64_t estid_t;

constexpr estid_t INVALID_ESTID = 0x0LLU;

#define EXCHANGE_ID_SHFE	"SHFE"	//上期所
#define EXCHANGE_ID_DCE		"DCE"	//大商所
#define EXCHANGE_ID_INE		"INE"	//能源中心
#define EXCHANGE_ID_ZCE		"ZCE"	//郑商所
#define EXCHANGE_ID_SGE		"SGE"	//广期所
#define EXCHANGE_ID_CFFEX	"CFFEX"	//中金所


struct tick_info;

struct bar_info;

struct tape_info;

struct position_info;

struct today_market_info;

enum class order_flag;

enum class offset_type;

enum class direction_type;

enum class event_type;

enum class error_type;

enum class deal_direction ;

enum class deal_status;

typedef std::function<bool(const code_t& code, offset_type offset, direction_type direction, uint32_t count, double_t price, order_flag flag)> filter_function;
