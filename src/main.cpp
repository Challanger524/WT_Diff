#include <rapidcsv.h>

#include <filesystem>
#include <exception>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <string>
#include <vector>
#include <format>
#include <map>

namespace fs = std::filesystem;
namespace rcv = rapidcsv;

using entries_t = std::vector<fs::directory_entry>;

constexpr auto EXTENSION_CSV = ".csv";
constexpr auto POSTFIX_DIFF = "-diff";

constexpr auto UNITS_CSV        = "units.csv";
constexpr auto UNITS_WEAPON_CSV = "units_weaponry.csv";

constexpr std::streamsize SETW_ID = 50;

//inline std::ostream &operator<<(std::ostream &os, const fs::file_time_type t) { return os << std::format("{:%F %T}", t); }
inline std::ostream &operator<<(std::ostream &os, const fs::file_time_type t) { return os << std::format("{:%F %R}:{:.2}", t, std::format("{:%S}", t)); }

class WtDiff {
	rcv::Document olderC; // C - csv
	rcv::Document newerC; // C - csv
	rcv::Document diff;
	std::map<std::string, size_t> olderM; // M - map <ID, pos>
	std::map<std::string, size_t> newerM; // M - map <ID, pos>

public:
	void operator()() {
		entries_t entries; // of csv
		entries.reserve(std::distance(fs::directory_iterator(fs::current_path()), fs::directory_iterator()));

		std::cout << "List of valid `.csv` files:\n";
		for (const auto &entry : fs::directory_iterator(fs::current_path())) {
			if (entry.path().extension() != EXTENSION_CSV)    continue;
			if (entry.path().string().contains(POSTFIX_DIFF)) continue;
			if (entry.path().string().contains("-skip"))      continue;

			entries.push_back(entry);
			auto ftime = entry.last_write_time();
			//std::cout << std::format("{:%F %T}", ftime) << " " << entry << "\n";
			//std::cout << std::format("{:%F %R}:{:.2}", ftime, std::format("{:%S}", ftime)) << " " << entry << "\n";
			std::cout << "  " << ftime << " " << entry << "\n";
		}

		entries_t units, weapons, other;
		for (const auto &entry : entries) {
			/**/ if (entry.path().string().contains(fs::path(UNITS_WEAPON_CSV).stem().string())) weapons.push_back(entry);
			else if (entry.path().string().contains(fs::path(UNITS_CSV       ).stem().string())) units  .push_back(entry);
			else                                                                                 other  .push_back(entry);
		}

		// clear: units_[weaponry/modifications] like entries
		for (size_t i = 0; i < units.size(); /*++*/) {
			if (units[i].path().string().contains("units_")) units.erase(units.begin() + i);
			else i++;
		}

		if (!weapons.empty()) Diff(weapons, UNITS_WEAPON_CSV);
		if (!units  .empty()) Diff(units  , UNITS_CSV       );
		if (!other  .empty()) Diff(other);

	}

private:
	void Diff(const entries_t &pair, std::string filename = std::string())
	{
		size_t c = 0;
		std::istringstream dummy_("");
		this->diff.Load(dummy_, rcv::LabelParams(0, -1), rcv::SeparatorParams(';', false, true, false, false));
		this->diff.Clear();
		this->diff.SetColumnName(c++, "<ID>");
		this->diff.SetColumnName(c++, "<ENG>");
		this->diff.SetColumnName(c++, "");

		olderM.clear();
		newerM.clear();

		std::cout << std::endl;

		if (pair.size() != 2) {
			std::cerr << "ERR: not a pair of csv files:\n";
			for (const auto &e : pair) std::cerr << "  " << e << "\n";
			return;
		}

		const fs::directory_entry &olderE = (pair[0].last_write_time() < pair[1].last_write_time()) ? pair[0] : pair[1]; // E - entry
		const fs::directory_entry &newerE = (pair[0].last_write_time() < pair[1].last_write_time()) ? pair[1] : pair[0]; // E - entry

		std::cout << "Processing next pair of files:\n";
		std::cout << "  older: " << olderE.last_write_time() << " " << olderE.path() << "\n";
		std::cout << "  newer: " << newerE.last_write_time() << " " << newerE.path() << "\n";

		this->olderC.Load(olderE.path().string(), rcv::LabelParams(), rcv::SeparatorParams(';', false, true, false, false));
		this->newerC.Load(newerE.path().string(), rcv::LabelParams(), rcv::SeparatorParams(';', false, true, false, false));

		Map(this->olderC, this->olderM);
		Map(this->newerC, this->newerM);

		DiffAdded  ();
		DiffRemoved();
		DiffChanged();

		fs::path save;
		if (filename.empty()) {
			const fs::path shortest = pair[0].path().string().size() < pair[1].path().string().size() ? pair[0] : pair[1];
			save = shortest.parent_path() / shortest.filename().stem().concat(POSTFIX_DIFF).replace_extension(".csv");
		}
		else // append "-diff" to the file name (preserve extension)
			save = fs::path(filename).replace_filename(fs::path(filename).stem().string() + std::string(POSTFIX_DIFF) + EXTENSION_CSV);

		this->diff.Save(save.string());
		std::cout << "Diff saved to: " << save << "\n";
	}

	static void Map(const rcv::Document &csv, std::map<std::string, size_t> &map) {
		std::cout << "Mapping units...";
		for (size_t r = 0; r < csv.GetRowCount(); r++) {
			const auto &row = csv.GetRow<std::string>(r);
			map[row[0]] = r;
		}
		std::cout << map.size() << " units mapped\n";
	}

	void DiffAdded()
	{
		this->diff.SetRow(diff.GetRowCount(), std::vector<std::string>{""});
		this->diff.SetRow(diff.GetRowCount(), std::vector<std::string>{"ADDED: <ID>, <ENG>"});
		//std::cout << "\n";
		//std::cout << "Diff: added:\n";

		for (size_t r = 0; r < this->newerC.GetRowCount(); r++)
		{
			auto row = this->newerC.GetRow<std::string>(r);

			if (row.size() > 1 && this->olderM.find(row[0]) == this->olderM.end()) {
				row.erase(row.begin() + 2, row.end());
				for (auto &s : row) Unbracket(s);
				SetwLeft(row[0]);

				this->diff.SetRow(diff.GetRowCount(), row);
				//std::cout << "  " << std::setw(SETW_ID) << std::left << row[0] << "; " << row[1] << "\n";
			}
		}
	}

	void DiffRemoved()
	{
		this->diff.SetRow(diff.GetRowCount(), std::vector<std::string>{""});
		this->diff.SetRow(diff.GetRowCount(), std::vector<std::string>{"REMOVED: <ID>, <ENG>"});
		//std::cout << "\n";
		//std::cout << "Diff: removed:\n";

		for (size_t r = 0; r < this->olderC.GetRowCount(); r++)
		{
			auto row = this->olderC.GetRow<std::string>(r);

			if (row.size() > 1 && this->newerM.find(row[0]) == this->newerM.end()) {
				row.erase(row.begin() + 2, row.end());
				for (auto &s : row) Unbracket(s);
				SetwLeft(row[0]);

				this->diff.SetRow(diff.GetRowCount(), row);
				//std::cout << "  " << std::setw(SETW_ID) << std::left << row[0] << "; " << row[1] << "\n";
			}
		}
	}

	void DiffChanged()
	{
		this->diff.SetRow(diff.GetRowCount(), std::vector<std::string>{""});
		this->diff.SetRow(diff.GetRowCount(), std::vector<std::string>{"CHANGED: <ID>, <ENG> OLD(upper)/NEW(lower)"});
		//std::cout << "\n";
		//std::cout << "Diff: changed:\n";

		for (size_t r = 0; r < this->newerC.GetRowCount(); r++)
		{
			auto newerRow = this->newerC.GetRow<std::string>(r);

			if (auto olderIt = this->olderM.find(newerRow[0]); newerRow.size() > 1 && olderIt != this->olderM.end())
			{
				auto olderRow = olderC.GetRow<std::string>(olderIt->second);
				if (newerRow.size() > 1 && olderRow.size() > 1 && newerRow[1] != olderRow[1])
				{
					newerRow.erase(newerRow.begin() + 2, newerRow.end());
					olderRow.erase(olderRow.begin() + 2, olderRow.end());

					if (newerRow.size() != 2 && newerRow.size() != 2) continue;

					for (auto &s : newerRow) Unbracket(s);
					for (auto &s : olderRow) Unbracket(s);
					SetwLeft(newerRow[0]);
					SetwLeft(olderRow[0]);

					this->diff.SetRow(diff.GetRowCount(), std::vector<std::string>{""});
					this->diff.SetRow(diff.GetRowCount(), olderRow);
					this->diff.SetRow(diff.GetRowCount(), newerRow);

					//std::cout << "  " << std::setw(SETW_ID) << std::left << olderRow[0] << "; " << olderRow[1] << " -> " << newerRow[1] << "\n";
				}
			}
		}
	}

	static void Unbracket(std::string &str) {
		if (!str.empty() && *str.rbegin() == '"') str.erase(str.size() - 1, 1);
		if (!str.empty() && *str. begin() == '"') str.erase(0             , 1);
	}

	[[nodiscard]] static std::string GetwLeft(const std::string &str) {
		std::stringstream sstr;
		sstr << std::setw(SETW_ID) << std::left << str;
		return sstr.str();
	}
	static void SetwLeft(std::string &str) {
		std::stringstream sstr;
		sstr << std::setw(SETW_ID) << std::left << str;
		str = sstr.str();
	}
};

int main()
{
	WtDiff wtDiff;

	try { wtDiff(); }
	catch(std::exception &e) { std::cerr << "EXCEP: " << e.what() << '\n'; }

#ifdef NDEBUG
	std::cout << "\nPress any button to exit...";
	std::cin.get();
#endif
}
