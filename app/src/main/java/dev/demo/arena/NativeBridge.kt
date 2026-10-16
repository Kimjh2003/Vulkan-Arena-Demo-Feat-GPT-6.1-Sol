package dev.demo.arena

import android.view.Surface
import java.nio.ByteBuffer

/** Only JNI downcalls; the default R8 native rule preserves these names. */
internal object NativeBridge {
    init { System.loadLibrary("arena") }
    external fun start(surface: Surface, manual: Boolean, weapon: Int, textureFamily: Int): String
    external fun prepare(): Int
    external fun frame(dt: Float, moveX: Float, moveZ: Float, aimX: Float, aimZ: Float,
        shooting: Boolean, dash: Boolean, upgrade: Int, state: ByteBuffer): Int
    external fun stop()
}
