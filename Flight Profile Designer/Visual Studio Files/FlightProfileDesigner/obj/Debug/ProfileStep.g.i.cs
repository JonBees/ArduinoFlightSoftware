﻿#pragma checksum "..\..\ProfileStep.xaml" "{406ea660-64cf-4c82-b6f0-42d48172a799}" "C4FD46C6548DE9F15BB9DEBBC56810A9"
//------------------------------------------------------------------------------
// <auto-generated>
//     This code was generated by a tool.
//     Runtime Version:4.0.30319.42000
//
//     Changes to this file may cause incorrect behavior and will be lost if
//     the code is regenerated.
// </auto-generated>
//------------------------------------------------------------------------------

using FlightProfileDesigner;
using System;
using System.Diagnostics;
using System.Windows;
using System.Windows.Automation;
using System.Windows.Controls;
using System.Windows.Controls.Primitives;
using System.Windows.Data;
using System.Windows.Documents;
using System.Windows.Ink;
using System.Windows.Input;
using System.Windows.Markup;
using System.Windows.Media;
using System.Windows.Media.Animation;
using System.Windows.Media.Effects;
using System.Windows.Media.Imaging;
using System.Windows.Media.Media3D;
using System.Windows.Media.TextFormatting;
using System.Windows.Navigation;
using System.Windows.Shapes;
using System.Windows.Shell;


namespace FlightProfileDesigner {
    
    
    /// <summary>
    /// ProfileStep
    /// </summary>
    public partial class ProfileStep : System.Windows.Controls.UserControl, System.Windows.Markup.IComponentConnector {
        
        
        #line 8 "..\..\ProfileStep.xaml"
        [System.Diagnostics.CodeAnalysis.SuppressMessageAttribute("Microsoft.Performance", "CA1823:AvoidUnusedPrivateFields")]
        internal FlightProfileDesigner.ProfileStep main;
        
        #line default
        #line hidden
        
        
        #line 20 "..\..\ProfileStep.xaml"
        [System.Diagnostics.CodeAnalysis.SuppressMessageAttribute("Microsoft.Performance", "CA1823:AvoidUnusedPrivateFields")]
        internal System.Windows.Controls.TextBox startPWM;
        
        #line default
        #line hidden
        
        
        #line 24 "..\..\ProfileStep.xaml"
        [System.Diagnostics.CodeAnalysis.SuppressMessageAttribute("Microsoft.Performance", "CA1823:AvoidUnusedPrivateFields")]
        internal System.Windows.Controls.TextBox endPWM;
        
        #line default
        #line hidden
        
        
        #line 27 "..\..\ProfileStep.xaml"
        [System.Diagnostics.CodeAnalysis.SuppressMessageAttribute("Microsoft.Performance", "CA1823:AvoidUnusedPrivateFields")]
        internal System.Windows.Controls.CheckBox sustainCheckBox;
        
        #line default
        #line hidden
        
        
        #line 31 "..\..\ProfileStep.xaml"
        [System.Diagnostics.CodeAnalysis.SuppressMessageAttribute("Microsoft.Performance", "CA1823:AvoidUnusedPrivateFields")]
        internal System.Windows.Controls.TextBox timePer;
        
        #line default
        #line hidden
        
        private bool _contentLoaded;
        
        /// <summary>
        /// InitializeComponent
        /// </summary>
        [System.Diagnostics.DebuggerNonUserCodeAttribute()]
        [System.CodeDom.Compiler.GeneratedCodeAttribute("PresentationBuildTasks", "4.0.0.0")]
        public void InitializeComponent() {
            if (_contentLoaded) {
                return;
            }
            _contentLoaded = true;
            System.Uri resourceLocater = new System.Uri("/FlightProfileDesigner;component/profilestep.xaml", System.UriKind.Relative);
            
            #line 1 "..\..\ProfileStep.xaml"
            System.Windows.Application.LoadComponent(this, resourceLocater);
            
            #line default
            #line hidden
        }
        
        [System.Diagnostics.DebuggerNonUserCodeAttribute()]
        [System.CodeDom.Compiler.GeneratedCodeAttribute("PresentationBuildTasks", "4.0.0.0")]
        [System.ComponentModel.EditorBrowsableAttribute(System.ComponentModel.EditorBrowsableState.Never)]
        [System.Diagnostics.CodeAnalysis.SuppressMessageAttribute("Microsoft.Design", "CA1033:InterfaceMethodsShouldBeCallableByChildTypes")]
        [System.Diagnostics.CodeAnalysis.SuppressMessageAttribute("Microsoft.Maintainability", "CA1502:AvoidExcessiveComplexity")]
        [System.Diagnostics.CodeAnalysis.SuppressMessageAttribute("Microsoft.Performance", "CA1800:DoNotCastUnnecessarily")]
        void System.Windows.Markup.IComponentConnector.Connect(int connectionId, object target) {
            switch (connectionId)
            {
            case 1:
            this.main = ((FlightProfileDesigner.ProfileStep)(target));
            return;
            case 2:
            this.startPWM = ((System.Windows.Controls.TextBox)(target));
            return;
            case 3:
            this.endPWM = ((System.Windows.Controls.TextBox)(target));
            return;
            case 4:
            this.sustainCheckBox = ((System.Windows.Controls.CheckBox)(target));
            
            #line 27 "..\..\ProfileStep.xaml"
            this.sustainCheckBox.Checked += new System.Windows.RoutedEventHandler(this.updateSustainBoolSCB);
            
            #line default
            #line hidden
            
            #line 27 "..\..\ProfileStep.xaml"
            this.sustainCheckBox.Unchecked += new System.Windows.RoutedEventHandler(this.updateSustainBoolSCB);
            
            #line default
            #line hidden
            return;
            case 5:
            this.timePer = ((System.Windows.Controls.TextBox)(target));
            return;
            }
            this._contentLoaded = true;
        }
    }
}
