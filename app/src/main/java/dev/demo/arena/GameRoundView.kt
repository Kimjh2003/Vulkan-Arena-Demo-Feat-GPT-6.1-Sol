package dev.demo.arena

import android.content.Context
import android.view.Gravity
import android.view.SurfaceView
import android.view.View
import android.widget.Button
import android.widget.FrameLayout

/** Owns round presentation and records each result once. */
internal class GameRoundView(context: Context, private val input: TouchInput,
    settings: ControlSettings, private val actions: RoundActions, private val progress: ProgressStore,
    retry: () -> Unit, home: () -> Unit) : FrameLayout(context) {
    val surface=SurfaceView(context)
    val overlay=FrameLayout(context)
    private val hud=GameHud(context)
    private val result: RoundResult
    private val upgrades: UpgradePanel
    private val dashButton: Button
    private var lastTenths=-1
    private var lastCooling=false
    private var recordSaved=false
    val finished: Boolean get()=result.visibility==VISIBLE
    init {
        addView(surface, FrameLayout.LayoutParams(-1, -1))
        val controls = VirtualControls(context, input, settings)
        addView(controls, FrameLayout.LayoutParams(-1, -1))
        overlay.addView(hud, FrameLayout.LayoutParams(-2, -2, Gravity.TOP or Gravity.START).apply {
            val gap = ArenaTheme.dp(context, 8)
            setMargins(gap, gap, gap, gap)
        })
        dashButton=ArenaTheme.button(context,"대시") {actions.requestDash()}
        overlay.addView(dashButton,FrameLayout.LayoutParams(ArenaTheme.dp(context,100),ArenaTheme.dp(context,48),Gravity.BOTTOM or Gravity.CENTER_HORIZONTAL).apply {
            bottomMargin=ArenaTheme.dp(context,12)
        })
        upgrades=UpgradePanel(context) {choice -> input.reset();actions.requestUpgrade(choice)}
        overlay.addView(upgrades,FrameLayout.LayoutParams(-1,-1))
        result = RoundResult(context, retry, home)
        overlay.addView(result, FrameLayout.LayoutParams(-1, -1))
        addView(overlay, FrameLayout.LayoutParams(-1, -1))
    }
    fun show(state: HudState) {
        hud.show(state)
        val terminal = state.ended || state.victory
        val enabled = state.dash <= 0 && !state.upgrading && !terminal
        if (dashButton.isEnabled != enabled) dashButton.isEnabled = enabled
        val tenths=UiText.dashTenths(state.dash)
        val cooling=state.dash>0
        if(tenths!=lastTenths || cooling!=lastCooling) {
            dashButton.text=if(cooling) UiText.dash(tenths) else "대시"
            lastTenths=tenths; lastCooling=cooling
        }
        upgrades.visibility = if(state.upgrading) View.VISIBLE else View.GONE
        if(state.upgrading) input.reset()
        if (terminal && result.visibility != View.VISIBLE) {
            input.reset()
            if(!recordSaved) { progress.record(state); recordSaved=true }
            dashButton.visibility=View.GONE; hud.visibility=View.GONE
            result.show(state)
        }
    }
    fun showError(error: String)=hud.showError(error)
}
