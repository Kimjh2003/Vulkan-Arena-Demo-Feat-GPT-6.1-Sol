package dev.demo.arena

import android.content.Context
import android.graphics.Typeface
import android.view.Gravity
import android.widget.Button
import android.widget.FrameLayout
import android.widget.LinearLayout
import android.widget.TextView
import java.util.Locale

/** Modal round summary blocks movement touches until the player chooses an action. */
internal class RoundResult(context: Context, retry: () -> Unit, home: () -> Unit) : FrameLayout(context) {
    private val title = TextView(context)
    private val summary = TextView(context)
    init {
        visibility = GONE
        isClickable = true
        setBackgroundColor(0xb3000000.toInt())
        val panel = LinearLayout(context).apply {
            orientation = LinearLayout.VERTICAL; gravity = Gravity.CENTER
            background = ArenaTheme.card(context)
            setPadding(dp(24), dp(16), dp(24), dp(16))
        }
        title.apply {
            textSize = 32f; gravity = Gravity.CENTER
            typeface = Typeface.create("sans-serif-black", Typeface.BOLD)
            accessibilityLiveRegion = ACCESSIBILITY_LIVE_REGION_POLITE
        }
        panel.addView(title)
        summary.apply {
            textSize = 16f; gravity = Gravity.CENTER; setTextColor(ArenaTheme.text)
            setPadding(0, dp(8), 0, dp(12))
        }
        panel.addView(summary)
        val actions = LinearLayout(context)
        for ((label, action) in listOf("다시 하기" to retry, "홈으로" to home)) {
            actions.addView(ArenaTheme.button(context,label,action), LinearLayout.LayoutParams(dp(130), dp(48)).apply {
                marginStart=dp(4);marginEnd=dp(4)
            })
        }
        panel.addView(actions)
        addView(panel, LayoutParams(-2, -2, Gravity.CENTER))
    }
    private fun dp(value: Int) = ArenaTheme.dp(context, value)
    fun show(state: HudState) {
        title.text = if (state.victory) "생존 성공!" else "게임 오버"
        title.setTextColor(if (state.victory) ArenaTheme.success else ArenaTheme.danger)
        val time = String.format(Locale.ROOT, "%02d:%02d", state.seconds / 60, state.seconds % 60)
        summary.text = "최종 점수 ${state.score}\n생존 시간 $time · 웨이브 ${state.wave}/5"
        visibility = VISIBLE
    }
}
