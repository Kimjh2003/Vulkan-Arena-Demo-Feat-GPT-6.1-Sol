package dev.demo.arena

import android.app.Activity
import android.os.Build
import android.os.Bundle
import android.view.SurfaceHolder
import android.view.SurfaceView
import android.view.WindowManager
import android.view.Gravity
import android.widget.TextView
import android.widget.LinearLayout
import android.window.OnBackInvokedCallback
import android.window.OnBackInvokedDispatcher

class MainActivity : Activity(), SurfaceHolder.Callback {
    private lateinit var surface: SurfaceView
    private lateinit var round: GameRoundView
    private val input = TouchInput()
    private val actions = RoundActions()
    private var session: RenderSession? = null
    private lateinit var store: SettingsStore
    private lateinit var graphicsSettings: GraphicsSettings
    private lateinit var audio: AudioSystem
    private var settings = ControlSettings()
    private var playing = false
    private lateinit var progress: ProgressStore

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
        if (playing && !round.finished && ::surface.isInitialized && surface.holder.surface.isValid && session == null) {
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
        WindowPresentation.applySafeInsets(box)
        WindowPresentation.setGameImmersive(window, true)
    }

    private fun play() {
        halt()
        playing = true
        actions.reset()
        input.reset()
        audio.enabled = settings.sound
        round=GameRoundView(this,input,settings,actions,progress,::play) {
            halt(); input.reset(); menu()
        }
        surface=round.surface
        surface.holder.addCallback(this)
        setContentView(round)
        WindowPresentation.applySafeInsets(round.overlay, false)
        WindowPresentation.setGameImmersive(window, true)
    }

    override fun surfaceCreated(holder: SurfaceHolder) = Unit

    override fun surfaceChanged(holder: SurfaceHolder, format: Int, width: Int, height: Int) {
        if (!::surface.isInitialized || holder !== surface.holder) return
        session?.let { if (it.isRunning) { it.refreshDisplay(); return } }
        halt()
        input.reset()
        if (!playing || round.finished) return
        val activeRound=round
        session = RenderSession(surface, input, actions, audio, settings.manualAim,
            progress.weapon, graphicsSettings.textureFamily,
            onState = { state -> if(playing && round === activeRound) activeRound.show(state) },
            onError = { error -> if(playing && round === activeRound) activeRound.showError(error) }
        ).also { it.start() }
    }

    private fun halt() {
        session?.stop()
        session = null
    }

    override fun surfaceDestroyed(holder: SurfaceHolder) {
        if (::surface.isInitialized && holder === surface.holder) halt()
    }

}
