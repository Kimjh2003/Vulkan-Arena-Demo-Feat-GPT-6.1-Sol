package dev.demo.arena

import android.content.Context
import android.graphics.Typeface
import android.widget.LinearLayout
import android.widget.TextView

/** Small corner readout; no full-width status bar. */
internal class GameHud(context: Context) : LinearLayout(context) {
    private val primary = TextView(context)
    private val ammo = TextView(context)
    private val health = TextView(context)
    private val detail = TextView(context)
    private val message = TextView(context)
    private var lastState: HudState? = null
    init {
        orientation = VERTICAL
        background = ArenaTheme.card(context, 0xd92c2c2a.toInt())
        setPadding(dp(10), dp(6), dp(10), dp(6))
        fun line(view: TextView, size: Float, color: Int) {
            view.textSize = size; view.setTextColor(color)
            addView(view)
        }
        line(primary, 15f, ArenaTheme.text)
        primary.typeface = Typeface.create("sans-serif-medium", Typeface.NORMAL)
        val vitals = LinearLayout(context).apply { orientation = HORIZONTAL }
        for (view in listOf(ammo, health)) {
            view.textSize = 13f
            vitals.addView(view, LayoutParams(-2, -2).apply { marginEnd = dp(12) })
        }
        addView(vitals)
        line(detail, 11f, ArenaTheme.secondary)
        line(message, 11f, ArenaTheme.warning)
        message.visibility = GONE
        primary.text = "준비 중…"
    }
    private fun dp(value: Int) = ArenaTheme.dp(context, value)
    fun show(state: HudState) {
        if (state == lastState) return
        val previous=lastState
        lastState=state
        var labelsChanged=false
        if(previous==null || previous.score!=state.score || previous.seconds!=state.seconds) {
            primary.text="점수 ${state.score}  ·  ${UiText.clock(state.seconds)}"; labelsChanged=true
        }
        if(previous==null || previous.ammo!=state.ammo || previous.magazine!=state.magazine) {
            ammo.text="탄약 ${state.ammo}/${state.magazine}"; labelsChanged=true
        }
        if(previous==null || previous.reloading!=state.reloading || previous.ammo!=state.ammo)
            ammo.setTextColor(if(state.reloading || state.ammo<=3) ArenaTheme.warning else ArenaTheme.info)
        if(previous==null || previous.hp!=state.hp) {
            health.text="체력 ${state.hp}/5"
            health.setTextColor(if(state.hp<=2) ArenaTheme.danger else ArenaTheme.success); labelsChanged=true
        }
        if(previous==null || previous.weapon!=state.weapon || previous.wave!=state.wave || previous.manual!=state.manual) {
            detail.text="${weaponNames[state.weapon.coerceIn(0,2)]} · 웨이브 ${state.wave}/5 · ${if(state.manual) "수동" else "자동"}"; labelsChanged=true
        }
        if(previous==null || previous.reloading!=state.reloading || previous.ended!=state.ended || previous.victory!=state.victory) {
            message.visibility=if(state.reloading && !state.ended && !state.victory) VISIBLE else GONE
            message.text="재장전 중…"
        }
        if(labelsChanged) contentDescription="${primary.text}, ${ammo.text}, ${health.text}, ${detail.text}"
    }
    fun showError(error: String) {
        message.visibility = VISIBLE
        message.text = "$error · 뒤로: 홈"
        message.setTextColor(ArenaTheme.danger)
    }
}
