# WT Diff

Diff-Compare old and new `lang/*.csv` files.

#### How to

0. Put two `.csv` files (like: `units.csv` and `units (1).csv`) in same folder with `.exe` program, where:
	- both filenames has common part in their names
	- one has older **Date modified** timestamp - treated as old
	- other has newer **Date modified** timestamp - treated as new
	- files with `-skip` and `-diff` in filenames are skipped
0. Run `WT_Diff.exe`
0. See new `<filename>-diff.csv` file

#### Other
- if you want to make old file newer - just open it, make change, save, revert change, save
- tested with next files: `units.csv`, `units_weaponry.csv`
- other filenames should be also supported (detected by new common name)

#### Console output example:
``` txt
./WT_Diff.exe
List of valid `.csv` files:
  2024-12-12 13:56:15 "C:\\dan\\dev\\wt_diff\\units (1).csv"
  2024-11-28 10:34:29 "C:\\dan\\dev\\wt_diff\\units.csv"
  2024-12-12 08:59:25 "C:\\dan\\dev\\wt_diff\\units_weaponry (1).csv"
  2024-12-12 08:54:05 "C:\\dan\\dev\\wt_diff\\units_weaponry.csv"

Processing next pair of files:
  older: 2024-12-12 08:54:05 "C:\\dan\\dev\\wt_diff\\units_weaponry.csv"
  newer: 2024-12-12 08:59:25 "C:\\dan\\dev\\wt_diff\\units_weaponry (1).csv"
Mapping units...6123 units mapped
Mapping units...6163 units mapped
Diff saved to: "units_weaponry-diff.csv"

Processing next pair of files:
  older: 2024-11-28 10:34:29 "C:\\dan\\dev\\wt_diff\\units.csv"
  newer: 2024-12-12 13:56:15 "C:\\dan\\dev\\wt_diff\\units (1).csv"
Mapping units...13603 units mapped
Mapping units...13765 units mapped
Diff saved to: "units-diff.csv"

Press any button to exit...
```
