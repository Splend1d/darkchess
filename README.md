# darkchess
Theory of Computer Games Homeworks

V1 alpha-beta logic 2W-18D, human evaluation tied positions : 4W2L 

V2 "obeserver" logic 1W-17D-2L , human evaluation tied positions : 9W2L 

V3 added branching score 10W-10D-0L

V4 added uncontested king special case, change branching score to branching count for both players at the last position (including Quiescent)

V5 branching score now considers importance of chess piece *17W-3D-0L

V6 resolved tie issues, changes to branching score 9W-11D

V7 resolved qsearch issues, added win more search, added eat first reward 15W-5D

V8 added closeness score when can't find something to eat, WWWWWWWWWWWDWDWDWDLD
- last 5 games, going second score : 5: WWDW 6:DDDD 7:DDLL 8:DWWD 9:WDWD

V8+ change Q search to "最後一個被吃位置吃到不能吃為止" 18W-2L
- last 4 games, going second score :  6:LLW 7:WWW 8:WWW 9:LLL

V9 added "晚被吃得分"，"早晚吃子" reward adjusted to take piece value into account, fix tie game bug 

V9-a: DRAW = LOSE - 19W-1D, 盤終draw = 0 (忘記+OFFSET)
- last 4 games, going second score 6:DL 7:W 8:W 9:W
*6黑方高估自己實力

V9-b: DRAW = -25000分for all scenario
- last 4 games, going second score 6:DDWDDW 7:DWWW 8:WWW 9:WDDD

V9-c: DRAW = -25000分for all scenario and repetitive detection
- all games going second score 3:DW, 4:DW, 6:DW, 8:LD, 9:WD
