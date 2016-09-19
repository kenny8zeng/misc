
    -ignorewarnings

    #指定代码的压缩级别
    -optimizationpasses 5

    #包明不混合大小写
    -dontusemixedcaseclassnames

    #不去忽略非公共的库类
    -dontskipnonpubliclibraryclasses

     #优化  不优化输入的类文件
    -dontoptimize

     #预校验
    -dontpreverify

     #混淆时是否记录日志
    -verbose

     # 混淆时所采用的算法
    -optimizations !code/simplification/arithmetic,!field/*,!class/merging/*

    #保护注解
    -keepattributes *Annotation*

    #保持哪些类不被混淆
    -keep public class * extends android.app.Fragment
    -keep public class * extends android.support.v4.app.Fragment
    -keep public class * extends android.app.Application

    #四大组件
    -keep public class * extends android.app.Activity
    -keep public class * extends android.app.Service
    -keep public class * extends android.content.BroadcastReceiver
    -keep public class * extends android.content.ContentProvider

    -keep public class * extends android.app.backup.BackupAgentHelper
    -keep public class * extends android.preference.Preference
    -keep public class com.android.vending.licensing.ILicensingService
    #资源文件
    -keep class **.R$* { *; }
    -keep class android.support.**{*;}

    -keep public class * extends android.view.View {
        public <init>(android.content.Context);
        public <init>(android.content.Context, android.util.AttributeSet);
        public <init>(android.content.Context, android.util.AttributeSet, int);
        public void set*(...);
    }

    #保持 native 方法不被混淆
    -keepclasseswithmembernames class * {
        native <methods>;
    }

    #保持自定义控件类不被混淆
    -keepclasseswithmembers class * {
        public <init>(android.content.Context, android.util.AttributeSet);
    }



    #保持 Parcelable 不被混淆
    -keep class * implements android.os.Parcelable {
      public static final android.os.Parcelable$Creator *;
    }

    #保持 Serializable 不被混淆
    -keepnames class * implements java.io.Serializable

    #保持 Serializable 不被混淆并且enum 类也不被混淆
    -keepclassmembers class * implements java.io.Serializable {
        static final long serialVersionUID;
        private static final java.io.ObjectStreamField[] serialPersistentFields;
        !static !transient <fields>;
        !private <fields>;
        !private <methods>;
        private void writeObject(java.io.ObjectOutputStream);
        private void readObject(java.io.ObjectInputStream);
        java.lang.Object writeReplace();
        java.lang.Object readResolve();
    }

    -keepclassmembers class * {
        public void *(android.view.View);
    }

    -keepclassmembers class * {
       public <init>(org.json.JSONObject);
    }