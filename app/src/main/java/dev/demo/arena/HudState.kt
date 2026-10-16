package dev.demo.arena

import java.nio.ByteBuffer

internal data class HudState @JvmOverloads constructor(
    val score: Int, val ammo: Int, val magazine: Int, val hp: Int,
    val seconds: Int, val wave: Int, val manual: Boolean,
    val reloading: Boolean, val ended: Boolean, val victory: Boolean,
    val weapon: Int = 0, val upgrading: Boolean = false, val dash: Float = 0f
) {
    companion object {
        const val WIRE_BYTES=14*4
        fun read(buffer: ByteBuffer): HudState = HudState(
            buffer.getInt(0),buffer.getInt(4),buffer.getInt(8),buffer.getInt(12),
            buffer.getInt(16),buffer.getInt(20),buffer.getInt(24)!=0,
            buffer.getInt(28)!=0,buffer.getInt(32)!=0,buffer.getInt(36)!=0,
            buffer.getInt(40),buffer.getInt(44)!=0,buffer.getFloat(48))
    }
}

