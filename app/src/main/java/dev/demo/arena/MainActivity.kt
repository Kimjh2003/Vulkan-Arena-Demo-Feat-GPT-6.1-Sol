package dev.demo.arena

import android.annotation.SuppressLint
import android.app.Activity
import android.os.Build
import android.os.Bundle
import android.view.Surface
import android.view.SurfaceHolder
import android.view.SurfaceView
import android.view.View
import android.view.WindowInsets
import android.view.WindowInsetsController
import android.view.WindowManager
import android.view.Gravity
import android.widget.FrameLayout
import android.widget.Button
import android.widget.TextView
import android.widget.LinearLayout
import java.util.concurrent.atomic.AtomicBoolean
import java.util.concurrent.atomic.AtomicInteger
import android.window.OnBackInvokedCallback
import android.window.OnBackInvokedDispatcher

class MainActivity : Activity(), SurfaceHolder.Callback {
    companion object {
        init {
            System.loadLibrary("arena")
        }
    }

    private external fun start(surface: Surface, manual: Boolean, weapon: Int, textureFamily: Int): String
    private external fun frame(dt: Float, moveX: Float, moveZ: Float,
                              aimX: Float, aimZ: Float, shooting: Boolean, dash: Boolean, upgrade: Int): String
    private external fun stop()

    private lateinit var surface: SurfaceView
    private lateinit var hud: GameHud
    private lateinit var result: RoundResult
    @Volatile private var running = false
    private val input = TouchInput()
    private var worker: Thread? = null
    private lateinit var store: SettingsStore
    private lateinit var graphicsSettings: GraphicsSettings
    private lateinit var audio: AudioSystem
    private var settings = ControlSettings()
    private var playing = false
    private lateinit var progress: ProgressStore
    private val dashRequest=AtomicBoolean(false)
    private val upgradeRequest=AtomicInteger(0)
    private lateinit var upgrades: UpgradePanel
    private lateinit var dashButton: Button
    private var recordSaved=false

    private var backCallback: OnBackInvokedCallback? = null

    override fun onCreate(state: Bundle?) {
        super.onCreate(state)
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.R) {
            window.setDecorFitsSystemWindows(false)
        }
        window.attributes = window.attributes.apply {
            layoutInDisplayCutoutMode = if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.R)
                WindowManager.LayoutParams.LAYOUT_IN_DISPLAY_CUTOUT_MODE_ALWAYS
            else WindowManager.LayoutParams.LAYOUT_IN_DISPLAY_CUTOUT_MODE_SHORT_EDGES
        }
        progress=ProgressStore(this)
        store = SettingsStore(this)
        graphicsSettings=GraphicsSettings(this)
        settings = store.load()
        audio = AudioSystem(this)
        setupBackHandler()
        menu()
    }

    private fun setupBackHandler() {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.TIRAMISU) {
            backCallback = OnBackInvokedCallback {
                handleBackPress()
            }
            onBackInvokedDispatcher.registerOnBackInvokedCallback(
                OnBackInvokedDispatcher.PRIORITY_DEFAULT,
                backCallback!!
            )
        }
    }

    private fun handleBackPress() {
        halt()
        input.reset()
        if (playing) menu() else finish()
    }

    @Deprecated("Deprecated in Java", ReplaceWith("super.onBackPressed()"))
    @Suppress("DEPRECATION", "OVERRIDE_DEPRECATION")
    override fun onBackPressed() {
        if (Build.VERSION.SDK_INT < Build.VERSION_CODES.TIRAMISU) {
            handleBackPress()
        } else {
            super.onBackPressed()
        }
    }

    override fun onDestroy() {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.TIRAMISU) {
            backCallback?.let {
                onBackInvokedDispatcher.unregisterOnBackInvokedCallback(it)
            }
        }
        halt()
        audio.release()
        super.onDestroy()
    }

    override fun onPause() {
        halt()
        input.reset()
        audio.pause()
        super.onPause()
    }

    override fun onResume() {
        super.onResume()
        audio.resume()
        if (playing && result.visibility != View.VISIBLE && ::surface.isInitialized && surface.holder.surface.isValid && worker == null) {
            surfaceChanged(surface.holder, 0, surface.width, surface.height)
        }
    }

    private fun menu() {
        playing = false
        val box = GameUi.menu(this, settings, ::play) {
            GameUi.settings(this, settings, onGraphics={GraphicsPicker.show(this,graphicsSettings)}) { saved ->
                settings = saved
                store.save(saved)
                menu()
            }
        }
        box.addView(WeaponPicker.button(this,progress.weapon) {selected ->
            progress.weapon=selected
            menu()
        },LinearLayout.LayoutParams(ArenaTheme.dp(this,280),ArenaTheme.dp(this,48)))
        box.addView(TextView(this).apply {text=progress.summary();gravity=Gravity.CENTER;textSize=12f;setTextColor(ArenaTheme.secondary)})
        setContentView(box)
        applySafeInsets(box)
        setGameImmersive(true)
    }

    @SuppressLint("ClickableViewAccessibility")
    private fun play() {
        halt()
        playing = true
        recordSaved=false;dashRequest.set(false);upgradeRequest.set(0)
        input.reset()
        audio.enabled = settings.sound
        val box = FrameLayout(this)
        hud = GameHud(this)
        surface = SurfaceView(this).apply {
            holder.addCallback(this@MainActivity)
        }
        box.addView(surface, FrameLayout.LayoutParams(-1, -1))
        val controls = VirtualControls(this, input, settings)
        box.addView(controls, FrameLayout.LayoutParams(-1, -1))
        val overlay = FrameLayout(this)
        overlay.addView(hud, FrameLayout.LayoutParams(-2, -2, Gravity.TOP or Gravity.START).apply {
            val gap = ArenaTheme.dp(this@MainActivity, 8)
            setMargins(gap, gap, gap, gap)
        })
        dashButton=ArenaTheme.button(this,"대시") {dashRequest.set(true)}
        overlay.addView(dashButton,FrameLayout.LayoutParams(ArenaTheme.dp(this,100),ArenaTheme.dp(this,48),Gravity.BOTTOM or Gravity.CENTER_HORIZONTAL).apply {
            bottomMargin=ArenaTheme.dp(this@MainActivity,12)
        })
        upgrades=UpgradePanel(this) {choice -> input.reset();upgradeRequest.set(choice)}
        overlay.addView(upgrades,FrameLayout.LayoutParams(-1,-1))
        result = RoundResult(this, ::play) { halt(); input.reset(); menu() }
        overlay.addView(result, FrameLayout.LayoutParams(-1, -1))
        box.addView(overlay, FrameLayout.LayoutParams(-1, -1))
        setContentView(box)
        // Only UI avoids the camera cutout; the render surface reaches every edge.
        applySafeInsets(overlay, false)
        setGameImmersive(true)
    }

    override fun surfaceCreated(holder: SurfaceHolder) = Unit

    override fun surfaceChanged(holder: SurfaceHolder, format: Int, width: Int, height: Int) {
        halt()
        input.reset()
        if (!playing || result.visibility == View.VISIBLE) return
        running = true
        val roundHud = hud
        val roundResult = result
        val roundUpgrades=upgrades
        var upgradeShown=false
        worker = Thread({
            try {
                val error = start(holder.surface, settings.manualAim, progress.weapon, graphicsSettings.textureFamily)
                if (error.isNotEmpty()) {
                    runOnUiThread { if (playing && hud === roundHud) roundHud.showError(error) }
                    return@Thread
                }
                var last = System.nanoTime()
                var count = 0
                var resultPosted = false
                while (running) {
                    val now = System.nanoTime()
                    val dt = (now - last) / 1e9f
                    last = now
                    val controls = input.current
                    val response = frame(dt, controls.moveX, controls.moveZ,
                                         controls.aimX, controls.aimZ, controls.shooting,dashRequest.getAndSet(false),upgradeRequest.getAndSet(0))
                    val separator = response.lastIndexOf('|')
                    val status = if (separator >= 0) response.substring(0, separator) else response
                    if (separator >= 0) audio.playEvents(response.substring(separator + 1).toIntOrNull() ?: 0)
                    if (!status.startsWith("ERROR")) {
                        val state = HudState.parse(status)
                        val terminal = state.ended || state.victory
                        if (!resultPosted && (++count % 10 == 0 || terminal || state.upgrading != upgradeShown)) {
                            if (terminal) resultPosted = true
                            upgradeShown=state.upgrading
                            runOnUiThread {
                                if (playing && hud === roundHud) {
                                    roundHud.show(state)
                                    dashButton.isEnabled=state.dash<=0 && !state.upgrading && !terminal
                                    dashButton.text=if(state.dash>0) "대시 %.1f".format(state.dash) else "대시"
                                    roundUpgrades.visibility=if(state.upgrading) View.VISIBLE else View.GONE
                                    if(state.upgrading) input.reset()
                                    if ((state.ended || state.victory) && roundResult.visibility != View.VISIBLE) {
                                        input.reset()
                                        if(!recordSaved) {progress.record(state);recordSaved=true}
                                        dashButton.visibility=View.GONE
                                        roundHud.visibility = View.GONE
                                        roundResult.show(state)
                                    }
                                }
                            }
                        }
                    }
                    if (status.startsWith("ERROR")) {
                        runOnUiThread { if (playing && hud === roundHud) roundHud.showError(status.removePrefix("ERROR: ")) }
                        break
                    }
                }
            } finally {
                stop()
            }
        }, "ArenaRenderer").also { it.start() }
    }

    private fun halt() {
        running = false
        worker?.let {
            try {
                it.join()
            } catch (_: InterruptedException) {
                Thread.currentThread().interrupt()
            }
        }
        worker = null
    }

    override fun surfaceDestroyed(holder: SurfaceHolder) = halt()

    @Suppress("DEPRECATION")
    private fun applySafeInsets(root: View, includeBars: Boolean = true) {
        root.setOnApplyWindowInsetsListener { view, insets ->
            if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.R) {
                val safe = insets.getInsets(WindowInsets.Type.displayCutout() or
                    if (includeBars) WindowInsets.Type.systemBars() else 0)
                view.setPadding(safe.left, safe.top, safe.right, safe.bottom)
            } else {
                val cutout = insets.displayCutout
                view.setPadding(maxOf(if (includeBars) insets.systemWindowInsetLeft else 0, cutout?.safeInsetLeft ?: 0),
                    maxOf(if (includeBars) insets.systemWindowInsetTop else 0, cutout?.safeInsetTop ?: 0),
                    maxOf(if (includeBars) insets.systemWindowInsetRight else 0, cutout?.safeInsetRight ?: 0),
                    maxOf(if (includeBars) insets.systemWindowInsetBottom else 0, cutout?.safeInsetBottom ?: 0))
            }
            insets
        }
        root.requestApplyInsets()
    }

    @Suppress("DEPRECATION")
    private fun setGameImmersive(enabled: Boolean) {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.R) {
            val bars = WindowInsets.Type.statusBars() or WindowInsets.Type.navigationBars()
            window.decorView.windowInsetsController?.let { controller ->
                if (enabled) {
                    controller.systemBarsBehavior =
                        WindowInsetsController.BEHAVIOR_SHOW_TRANSIENT_BARS_BY_SWIPE
                    controller.hide(bars)
                } else {
                    controller.show(bars)
                }
            }
        } else {
            window.decorView.systemUiVisibility = if (enabled) {
                View.SYSTEM_UI_FLAG_FULLSCREEN or View.SYSTEM_UI_FLAG_HIDE_NAVIGATION or
                    View.SYSTEM_UI_FLAG_IMMERSIVE_STICKY or View.SYSTEM_UI_FLAG_LAYOUT_STABLE or
                View.SYSTEM_UI_FLAG_LAYOUT_FULLSCREEN or View.SYSTEM_UI_FLAG_LAYOUT_HIDE_NAVIGATION
            } else {
                View.SYSTEM_UI_FLAG_VISIBLE
            }
        }
    }
}
