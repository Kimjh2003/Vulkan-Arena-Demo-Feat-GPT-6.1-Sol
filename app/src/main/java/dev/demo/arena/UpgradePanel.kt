package dev.demo.arena
import android.content.Context
import android.view.Gravity
import android.widget.Button
import android.widget.FrameLayout
import android.widget.LinearLayout
import android.widget.TextView
internal class UpgradePanel(context: Context, choose: (Int)->Unit): FrameLayout(context) {
    init {
        visibility=GONE;isClickable=true;setBackgroundColor(0xb3000000.toInt())
        val panel=LinearLayout(context).apply {
            orientation=LinearLayout.VERTICAL;gravity=Gravity.CENTER
            background=ArenaTheme.card(context);setPadding(ArenaTheme.dp(context,16),ArenaTheme.dp(context,12),ArenaTheme.dp(context,16),ArenaTheme.dp(context,12))
        }
        panel.addView(TextView(context).apply {
            text="웨이브 돌파! 업그레이드 선택";textSize=22f;setTextColor(ArenaTheme.success)
        })
        panel.addView(TextView(context).apply {
            text="선택하는 동안 전투는 정지 · 강화는 이번 판에 누적";textSize=12f;setTextColor(ArenaTheme.text)
        })
        for((i,label) in listOf("공격력 +1", "발사 간격 −15%", "이동 속도 +15%").withIndex()) {
            panel.addView(ArenaTheme.button(context,label) {
                this@UpgradePanel.visibility=GONE;choose(i+1)
            },LinearLayout.LayoutParams(-1,ArenaTheme.dp(context,48)).apply {
                topMargin=ArenaTheme.dp(context,6)
            })
        }
        addView(panel,LayoutParams(-2,-2,Gravity.CENTER))
    }
}

