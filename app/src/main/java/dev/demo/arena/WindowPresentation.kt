package dev.demo.arena

import android.os.Build
import android.view.View
import android.view.WindowInsets
import android.view.WindowInsetsController

internal object WindowPresentation {
    @Suppress("DEPRECATION")
    fun applySafeInsets(root: View, includeBars: Boolean = true) {
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
    fun setGameImmersive(window: android.view.Window, enabled: Boolean) {
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
