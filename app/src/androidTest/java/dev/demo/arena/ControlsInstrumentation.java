package dev.demo.arena;

import android.app.Instrumentation;
import android.os.Bundle;
import android.os.SystemClock;
import android.view.MotionEvent;
import android.view.View;
import android.view.ViewGroup;
import android.view.KeyEvent;
import android.content.Intent;
import android.app.Activity;
import android.widget.TextView;
import java.util.concurrent.Callable;
import android.view.SurfaceView;

/** Framework-only integration checks; no test-library dependency. */
public final class ControlsInstrumentation extends Instrumentation {
    @Override public void onCreate(Bundle args) { super.onCreate(args); start(); }
    private static void check(boolean condition, String message) {
        if (!condition) throw new AssertionError(message);
    }
    private static Thread renderOwner() {
        Thread owner=null;
        for(Thread t:Thread.getAllStackTraces().keySet()) if(t.isAlive() && t.getName().equals("ArenaRenderer")) {
            check(owner==null,"Overlapping render workers");owner=t;
        }
        check(owner!=null,"Missing render worker");return owner;
    }
    private static View findText(View view, String text) {
        if(view instanceof TextView && ((TextView)view).getText().toString().equals(text)) return view;
        if(view instanceof ViewGroup) {
            ViewGroup group=(ViewGroup)view;
            for(int i=0;i<group.getChildCount();i++) {
                View found=findText(group.getChildAt(i),text); if(found!=null) return found;
            }
        }
        return null;
    }
    private void awaitUi(Callable<Boolean> condition, String failure) throws Exception {
        long deadline=SystemClock.uptimeMillis()+15000;
        while(SystemClock.uptimeMillis()<deadline) {
            final boolean[] result={false}; final Exception[] error={null};
            runOnMainSync(() -> {try {result[0]=condition.call();} catch(Exception e) {error[0]=e;}});
            if(error[0]!=null) throw error[0];
            if(result[0]) return;
            SystemClock.sleep(100);
        }
        throw new AssertionError(failure);
    }
    private static SurfaceView findSurface(View view) {
        if(view instanceof SurfaceView) return (SurfaceView)view;
        if(view instanceof ViewGroup) {
            ViewGroup group=(ViewGroup)view;
            for(int i=0;i<group.getChildCount();i++) {
                SurfaceView found=findSurface(group.getChildAt(i));if(found!=null)return found;
            }
        }
        return null;
    }
    private void awaitFrames(boolean active) throws Exception {
        long deadline=SystemClock.uptimeMillis()+120000;
        while(SystemClock.uptimeMillis()<deadline) {
            long frames=NativeDiagnostics.INSTANCE.renderedFrames();
            if(active?frames>=3:frames==0)return;
            SystemClock.sleep(100);
        }
        throw new AssertionError(active?"Vulkan did not present three frames":"Paused renderer remained active");
    }
    private void awaitGeneration(long previous) throws Exception {
        long deadline=SystemClock.uptimeMillis()+120000;
        while(SystemClock.uptimeMillis()<deadline) {
            if(NativeDiagnostics.INSTANCE.rendererGeneration()>previous)return;
            SystemClock.sleep(100);
        }
        throw new AssertionError("Surface change did not recreate the Vulkan renderer");
    }
    private static MotionEvent event(int action, int[] ids, float[] xs, float[] ys) {
        MotionEvent.PointerProperties[] properties=new MotionEvent.PointerProperties[ids.length];
        MotionEvent.PointerCoords[] coords=new MotionEvent.PointerCoords[ids.length];
        for(int i=0;i<ids.length;i++) {
            properties[i]=new MotionEvent.PointerProperties();properties[i].id=ids[i];
            properties[i].toolType=MotionEvent.TOOL_TYPE_FINGER;
            coords[i]=new MotionEvent.PointerCoords();coords[i].x=xs[i];coords[i].y=ys[i];
            coords[i].pressure=1;coords[i].size=1;
        }
        long now=SystemClock.uptimeMillis();
        return MotionEvent.obtain(now,now,action,ids.length,properties,coords,0,0,1,1,0,0,0,0);
    }
    private void send(TouchInput input,View view,ControlSettings settings,int action,
                      int[] ids,float[] xs,float[] ys) {
        MotionEvent e=event(action,ids,xs,ys);
        try {input.onTouch(view,e,settings);} finally {e.recycle();}
    }
    @Override public void onStart() {
        Bundle results=new Bundle();
        try {
            final Throwable[] uiFailure={null};
            runOnMainSync(() -> {
                try {
                View view=new View(getTargetContext());view.layout(0,0,2000,1000);
                ControlSettings manual=new ControlSettings(true,false,true);
                TouchInput input=new TouchInput();
                send(input,view,manual,MotionEvent.ACTION_DOWN,new int[]{7},new float[]{460},new float[]{790});
                check(input.getCurrent().getMoveX()>.5f,"Movement stick did not engage");
                InputFrame original=input.getCurrent();
                send(input,view,manual,MotionEvent.ACTION_MOVE,new int[]{7},new float[]{460},new float[]{790});
                check(input.getCurrent()==original,"Identical input allocated another snapshot");
                send(input,view,manual,MotionEvent.ACTION_POINTER_DOWN | (1<<8),
                     new int[]{7,13},new float[]{460,1640},new float[]{790,650});
                check(input.getCurrent().getShooting() && input.getCurrent().getAimZ()<-.9f,
                      "Second pointer did not engage aim/fire");
                check(input.getCurrent().getMoveX()>.5f,"Aiming interrupted movement");
                send(input,view,manual,MotionEvent.ACTION_POINTER_UP,
                     new int[]{7,13},new float[]{460,1640},new float[]{790,650});
                check(input.getCurrent().getMoveX()==0 && input.getCurrent().getShooting(),
                      "Releasing movement also released aim");
                send(input,view,manual,MotionEvent.ACTION_CANCEL,
                     new int[]{13},new float[]{1640},new float[]{650});
                check(!input.getCurrent().getShooting() && input.getCurrent().getAimZ()==0,
                      "Cancelled touch remained active");
                ControlSettings auto=new ControlSettings(false,false,true);
                send(input,view,auto,MotionEvent.ACTION_DOWN,new int[]{2},new float[]{1640},new float[]{650});
                check(!input.getCurrent().getShooting() && input.getCurrent().getMoveX()==0,
                      "Auto mode accepted an aim pointer");
                input.reset();
                ControlSettings mirrored=new ControlSettings(true,true,false);
                send(input,view,mirrored,MotionEvent.ACTION_DOWN,new int[]{3},new float[]{1740},new float[]{790});
                check(input.getCurrent().getMoveX()>.5f,"Mirrored movement stick failed");
                send(input,view,mirrored,MotionEvent.ACTION_POINTER_DOWN | (1<<8),
                     new int[]{3,4},new float[]{1740,360},new float[]{790,650});
                check(input.getCurrent().getShooting(),"Mirrored aiming stick failed");
                input.reset();
                SettingsStore store=new SettingsStore(getTargetContext());
                ControlSettings before=store.load();
                try {
                    store.save(mirrored);
                    ControlSettings saved=new SettingsStore(getTargetContext()).load();
                    check(saved.getManualAim() && saved.getLeftHanded() && !saved.getSound(),
                          "Settings roundtrip failed");
                } finally {store.save(before);}
                java.nio.ByteBuffer wire=java.nio.ByteBuffer.allocateDirect(56).order(java.nio.ByteOrder.nativeOrder());
                int[] fields={120,0,12,2,65,2,1,1,0,0,2,1,0,3};
                for(int i=0;i<fields.length;i++) wire.putInt(i*4,fields[i]);
                wire.putFloat(48,1.234567f);
                HudState state=HudState.Companion.read(wire);
                check(state.getWeapon()==2 && state.getUpgrading() && state.getDash()==1.234567f,
                      "Binary HUD state lost optional fields or float precision");

                NativeStateBuffer buffered=new NativeStateBuffer();
                for(int i=0;i<14;i++) buffered.getBytes().putInt(i*4,wire.getInt(i*4));
                check(buffered.changedSnapshot().getDash()==1.234567f,"Snapshot changed float precision");
                check(buffered.changedSnapshot()==null,"Identical wire state republished");
                buffered.getBytes().putInt(52,99);
                check(buffered.getEvents()==99 && buffered.changedSnapshot()==null,"Audio events forced UI publication");
                buffered.getBytes().putInt(0,121);
                check(buffered.changedSnapshot().getScore()==121,"Changed score was dropped");
                check(UiText.INSTANCE.clock(181).equals("03:01"),"Shared clock formatting failed");
                check(UiText.INSTANCE.dash(12).equals("대시 1.2"),"Dash formatting failed");
                GameHud hud=new GameHud(getTargetContext());hud.show(state);
                check(findText(hud,"점수 120  ·  01:05")!=null && findText(hud,"탄약 0/12")!=null,
                      "Structured HUD values were not displayed");
                check(findText(hud,"재장전 중…")!=null,"Reload state was not displayed");
                final int[] actions={0,0};
                RoundResult result=new RoundResult(getTargetContext(),
                    () -> {actions[0]++;return kotlin.Unit.INSTANCE;},
                    () -> {actions[1]++;return kotlin.Unit.INSTANCE;});
                check(result.getVisibility()==View.GONE,"Result appeared before the round ended");
                result.show(new HudState(900,11,12,0,78,2,false,false,true,false));
                check(result.getVisibility()==View.VISIBLE && result.isClickable() && findText(result,"게임 오버")!=null,
                      "Game over modal missing");
                check(findText(result,"최종 점수 900\n생존 시간 01:18 · 웨이브 2/5")!=null,"Final record missing");
                findText(result,"다시 하기").performClick();findText(result,"홈으로").performClick();
                check(actions[0]==1 && actions[1]==1,"Result actions failed");
                final int[] upgrade={0};
                UpgradePanel choices=new UpgradePanel(getTargetContext(),value -> {upgrade[0]=value;return kotlin.Unit.INSTANCE;});
                choices.setVisibility(View.VISIBLE);findText(choices,"공격력 +1").performClick();
                check(upgrade[0]==1 && choices.getVisibility()==View.GONE,"Upgrade choice action failed");
                android.content.SharedPreferences prefs=getTargetContext().getSharedPreferences("progress",0);
                java.util.Map<String,?> old=prefs.getAll();
                try {
                    prefs.edit().clear().commit();
                    ProgressStore progress=new ProgressStore(getTargetContext());progress.setWeapon(2);
                    progress.record(new HudState(900,12,12,0,78,2,false,false,true,false));
                    progress.record(new HudState(100,12,12,0,20,1,false,false,true,false));
                    check(prefs.getInt("score",0)==900 && prefs.getInt("seconds",0)==78,"Best record regressed");
                    check(new ProgressStore(getTargetContext()).getWeapon()==2,"Weapon selection was not saved");
                } finally {
                    android.content.SharedPreferences.Editor restore=prefs.edit().clear();
                    for(java.util.Map.Entry<String,?> entry:old.entrySet()) {
                        Object value=entry.getValue();
                        if(value instanceof Integer) restore.putInt(entry.getKey(),(Integer)value);
                    }
                    restore.commit();
                }
                result.show(new HudState(1200,12,12,3,300,5,false,false,false,true));
                check(findText(result,"생존 성공!")!=null,"Victory modal missing");
                } catch(Throwable error) {uiFailure[0]=error;}
            });
            if(uiFailure[0]!=null) throw uiFailure[0];

            final int[] deliveries={0}; final HudState[] delivered={null};
            final StateMailbox[] mailbox={null};
            HudState first=new HudState(1,12,12,5,0,1,false,false,false,false);
            HudState latest=new HudState(2,12,12,5,0,1,false,false,false,false);
            runOnMainSync(() -> {
                mailbox[0]=new StateMailbox(value -> { deliveries[0]++;delivered[0]=value;return kotlin.Unit.INSTANCE; });
                mailbox[0].offer(first);mailbox[0].offer(latest);
            });
            waitForIdleSync();
            check(deliveries[0]==1 && delivered[0]==latest,"Mailbox did not coalesce pending UI states");
            runOnMainSync(() -> {mailbox[0].offer(first);mailbox[0].close();});
            waitForIdleSync();
            check(deliveries[0]==1,"Closed round delivered stale UI state");
            Activity activity=startActivitySync(new Intent(getTargetContext(),MainActivity.class).addFlags(Intent.FLAG_ACTIVITY_NEW_TASK));
            awaitUi(() -> findText(activity.getWindow().getDecorView(),"플레이")!=null,"Home play action missing");
            runOnMainSync(() -> findText(activity.getWindow().getDecorView(),"플레이").performClick());
            // SurfaceView may gain focus only after its first buffer is ready.
            // Cold software-GPU initialization uses the existing frame deadline;
            // still require both actual Vulkan frames and window focus.
            awaitFrames(true);
            awaitUi(() -> findText(activity.getWindow().getDecorView(),"다시 하기")!=null && activity.hasWindowFocus(),"Gameplay did not gain focus");
            long initialGeneration=NativeDiagnostics.INSTANCE.rendererGeneration();
            Thread initialOwner=renderOwner();
            runOnMainSync(() -> findSurface(activity.getWindow().getDecorView()).getHolder().setFixedSize(960,540));
            awaitGeneration(initialGeneration);
            awaitFrames(true);
            check(renderOwner()==initialOwner,"Resize replaced the simulation/render owner");
            long resizedGeneration=NativeDiagnostics.INSTANCE.rendererGeneration();
            runOnMainSync(() -> findSurface(activity.getWindow().getDecorView()).getHolder().setSizeFromLayout());
            awaitGeneration(resizedGeneration);
            awaitFrames(true);
            check(renderOwner()==initialOwner,"Surface restoration replaced the render owner");
            runOnMainSync(() -> activity.moveTaskToBack(true));
            awaitFrames(false);
            getTargetContext().startActivity(new Intent(getTargetContext(),MainActivity.class)
                .addFlags(Intent.FLAG_ACTIVITY_NEW_TASK|Intent.FLAG_ACTIVITY_REORDER_TO_FRONT));
            awaitFrames(true);
            renderOwner();
            waitForIdleSync();
            sendKeyDownUpSync(KeyEvent.KEYCODE_BACK);waitForIdleSync();
            awaitUi(() -> !activity.isFinishing() && findText(activity.getWindow().getDecorView(),"플레이")!=null,
                    "Gameplay Back did not return to home");
            sendKeyDownUpSync(KeyEvent.KEYCODE_BACK);waitForIdleSync();
            awaitUi(() -> activity.isFinishing() || activity.isDestroyed(),"Home Back did not finish the activity");
            results.putString("stream","Integration checks passed: controls, settings, HUD, Vulkan frames, resize, pause/resume, gameplay Back, home exit.\n");
            finish(-1,results);
        } catch(Throwable failure) {
            results.putString("stream","FAILED: "+failure+"\n");finish(1,results);
        }
    }
}
