�������0� �0�0�����K�����L���a�����*���Y��p����������b�������������;��V�����������������������@������Y���Y�Y�Y�Y���7�������������������%��
����������������
����s��	����������)�C�]����t������������&�+�'�D'��'�+��'�+�.(�+�+��(��(�	)��)��)��)�Z*��*�+�+��*��)�/��*��*�,/��*��*��)�?/�ÿ ξ ν ý μ ü ∼ λ û κ ú ι ù θ Ÿ η ŷ ζ Ŷ ö ε õ δ ô γ ó ỳ ″ β ò ′ α ñ ± ° ï î ® í ì ë ∫ ê Ω é © Ψ è ç § Φ Υ å ⊥ ≥ ä ≤ Σ ã â ™ á ≡ Π à   ≠ ß Ξ ∞ Ý ∝ ” Ü “ Λ Û Ú √ Ù ’ Θ ‘ × Ö Õ Δ Ô — Γ Ó – Ò − → Ñ ∑ ← Ï ∏ Î Í Ì Ë ⌋ Ê ⌊ ω É ⌉ ∉ ψ È ⌈ ≈ ∈ χ Ç ∇ φ υ Å ⋅ ∅ τ Ä σ Ã ς Â ∂ ρ Á π À 	
 ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789!"#$%&'()*+,-./:;<=>?@[\]^_`{|}~ operator~ ->~ .~ }} ,:?[]{} @{}  {} ={@ccc},r,r,~{cc},~{memory} {cx} ={eax} {0:x} ~{dirflag},~{fpsr},~{flags} {rdi} string must begin with rv32{i,e,g} or rv64{i,g} f} ${:uid} {0:d} ]}  \,\} :P} {objc_object=} {objc_class=} -{0:x8} +{0:x8} {0:8} {0:x6} {0:6} {0:x16} -- {1:x16} {0:x16} $R{0:X-6} [{0}:{1}, byte={2}]: {3} {0:3} {0:x2} label="{0:P}" penwidth={1} label="W:{0}" penwidth={1} {0} #{1} {0:x1} ~{$1} arg{0} --new-line-format={0} --old-line-format={0} --unchanged-line-format={0} #{0} {x0} {w0} {r0} #} { } { unroll[x]: 1 = 0 } { ... } " } Finds and renames qualified symbols in code within a translation unit.
It is used to move/rename a symbol to a new namespace/name:
  * Supported symbols: classes, class members, functions, enums, and type alias.
  * Renames all symbol occurrences from the old qualified name to the new
    qualified name. All symbol references will be correctly qualified; For
    symbol definitions, only name will be changed.
For example, rename "A::Foo" to "B::Bar":
  Old code:
    namespace foo {
    class A {};
    }

    namespace bar {
    void f(foo::A a) {}
    }

  New code after rename:
    namespace foo {
    class B {};
    }

    namespace bar {
    void f(B b) {}
    } operator|| operator| $|  | ~{ {{ Element{ raw_offset{ temp_object{ SectionedAddress{ pointerToMember{ Ivar{ SymRegion{ \mathrm{ compoundVal{ lazyCompoundVal{ f{ Base{ block_code{ StaticGlobalsMemSpace{ Derived{ block_data{ @{ Initial VPlan for VF={ ":{ &{ jmp ${ label="{  = !{ ; assume predicate info { var relevant_lines = { , intoperands = { set-state(< { operands: { "program_state": { , { q8Usq8Usq8Uszz q8Usq8bq8Usq8Uszz q8Ucq8bq8Ucq8Uszz q16Usq16Usq16Uszz q16Usq16bq16Usq16Uszz q16Ucq16bq16Ucq16Uszz q4Usq4Usq4Uszz q4Usq4bq4Usq4Uszz q4Ucq4bq4Ucq4Uszz q2Usq2Usq2Uszz q2Usq2bq2Usq2Uszz q2Ucq2bq2Ucq2Uszz q32Usq32Usq32Uszz q32Usq32bq32Usq32Uszz q32Ucq32bq32Ucq32Uszz q1Usq1Usq1Uszz q1Usq1bq1Usq1Uszz q1Ucq1bq1Ucq1Uszz q8Ssq8Ssq8Sszz q8Ssq8bq8Ssq8Sszz q8Scq8bq8Scq8Sszz q16Ssq16Ssq16Sszz q16Ssq16bq16Ssq16Sszz q16Scq16bq16Scq16Sszz q4Ssq4Ssq4Sszz q4Ssq4bq4Ssq4Sszz q4Scq4bq4Scq4Sszz q2Ssq2Ssq2Sszz q2Ssq2bq2Ssq2Sszz q2Scq2bq2Scq2Sszz q32Ssq32Ssq32Sszz q32Ssq32bq32Ssq32Sszz q32Scq32bq32Scq32Sszz q1Ssq1Ssq1Sszz q1Ssq1bq1Ssq1Sszz q1Scq1bq1Scq1Sszz q8UWiq8UWiq8UWizz q8UWiq8bq8UWiq8UWizz q8Uiq8bq8Uiq8UWizz q4UWiq4UWiq4UWizz q4UWiq4bq4UWiq4UWizz q4Uiq4bq4Uiq4UWizz q2UWiq2UWiq2UWizz q2UWiq2bq2UWiq2UWizz q2Uiq2bq2Uiq2UWizz q1UWiq1UWiq1UWizz q1UWiq1bq1UWiq1UWizz q1Uiq1bq1Uiq1UWizz q8SWiq8SWiq8SWizz q8SWiq8bq8SWiq8SWizz q8Siq8bq8Siq8SWizz q4SWiq4SWiq4SWizz q4SWiq4bq4SWiq4SWizz q4Siq4bq4Siq4SWizz q2SWiq2SWiq2SWizz q2SWiq2bq2SWiq2SWizz q2Siq2bq2Siq2SWizz q1SWiq1SWiq1SWizz q1SWiq1bq1SWiq1SWizz q1Siq1bq1Siq1SWizz q8Usq8bq8Usq8Uizz q8Uiq8Uiq8Uizz q8Uiq8bq8Uiq8Uizz q16Usq16bq16Usq16Uizz q16Uiq16Uiq16Uizz q16Uiq16bq16Uiq16Uizz q4Usq4bq4Usq4Uizz q4Uiq4Uiq4Uizz q4Uiq4bq4Uiq4Uizz q2Usq2bq2Usq2Uizz q2Uiq2Uiq2Uizz q2Uiq2bq2Uiq2Uizz q1Usq1bq1Usq1Uizz q1Uiq1Uiq1Uizz q1Uiq1bq1Uiq1Uizz q8Ssq8bq8Ssq8Sizz q8Siq8Siq8Sizz q8Siq8bq8Siq8Sizz q16Ssq16bq16Ssq16Sizz q16Siq16Siq16Sizz q16Siq16bq16Siq16Sizz q4Ssq4bq4Ssq4Sizz